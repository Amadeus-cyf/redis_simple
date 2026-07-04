#include "data_types/hash/hash.h"

#include <cassert>
#include <stdexcept>
#include <utility>
#include <vector>

namespace redis_simple::hash {
Hash::Hash()
    : encoding_(Encoding::kListPack),
      listpack_(std::make_unique<in_memory::ListPack>()),
      dict_(nullptr) {}

bool Hash::Set(const std::string& field, const std::string& value) {
  if (encoding_ == Encoding::kListPack) {
    return SetListPack(field, value);
  }
  if (encoding_ == Encoding::kDict) {
    return SetDict(field, value);
  }
  throw std::invalid_argument("unknown hash encoding type");
}

std::optional<std::string> Hash::Get(const std::string& field) const {
  if (encoding_ == Encoding::kListPack) {
    const auto field_idx = FindListPackField(field);
    if (!field_idx.has_value()) {
      return std::nullopt;
    }
    const ssize_t value_idx =
        listpack_->Next(static_cast<size_t>(*field_idx));
    if (value_idx < 0) {
      return std::nullopt;
    }
    return listpack_->Get(static_cast<size_t>(value_idx));
  }
  if (encoding_ == Encoding::kDict) {
    const auto* value = dict_->FindValue(field);
    return value == nullptr ? std::nullopt : std::optional<std::string>(*value);
  }
  throw std::invalid_argument("unknown hash encoding type");
}

bool Hash::Delete(const std::string& field) {
  if (encoding_ == Encoding::kListPack) {
    const auto field_idx = FindListPackField(field);
    if (!field_idx.has_value()) {
      return false;
    }
    const auto idx = static_cast<size_t>(*field_idx);
    listpack_->Delete(idx);
    listpack_->Delete(idx);
    return true;
  }
  if (encoding_ == Encoding::kDict) {
    return dict_->Delete(field);
  }
  throw std::invalid_argument("unknown hash encoding type");
}

bool Hash::Exists(const std::string& field) const {
  if (encoding_ == Encoding::kListPack) {
    return FindListPackField(field).has_value();
  }
  if (encoding_ == Encoding::kDict) {
    return dict_->FindValue(field) != nullptr;
  }
  throw std::invalid_argument("unknown hash encoding type");
}

size_t Hash::Size() const {
  if (encoding_ == Encoding::kListPack) {
    return listpack_ == nullptr ? 0 : listpack_->Size() / 2;
  }
  if (encoding_ == Encoding::kDict) {
    return dict_ == nullptr ? 0 : dict_->Size();
  }
  throw std::invalid_argument("unknown hash encoding type");
}

std::vector<Hash::Entry> Hash::Entries() const {
  if (encoding_ == Encoding::kListPack) {
    return ListPackEntries();
  }
  if (encoding_ == Encoding::kDict) {
    return DictEntries();
  }
  throw std::invalid_argument("unknown hash encoding type");
}

bool Hash::CanStoreInListPack(const std::string& field,
                              const std::string& value) {
  return field.size() <= kListPackMaxElementLength &&
         value.size() <= kListPackMaxElementLength;
}

bool Hash::CanAppendToListPack(const std::string& field,
                               const std::string& value) const {
  return Size() < kListPackMaxEntries && CanStoreInListPack(field, value) &&
         in_memory::ListPack::SafeToAdd(listpack_.get(),
                                        field.size() + value.size());
}

bool Hash::SetListPack(const std::string& field, const std::string& value) {
  assert(encoding_ == Encoding::kListPack);
  const auto field_idx = FindListPackField(field);
  if (field_idx.has_value()) {
    if (!CanStoreInListPack(field, value)) {
      ConvertListPackToDict(Size());
      SetDict(field, value);
      return false;
    }
    const ssize_t value_idx =
        listpack_->Next(static_cast<size_t>(*field_idx));
    if (value_idx < 0) {
      return false;
    }
    listpack_->Replace(static_cast<size_t>(value_idx), value);
    return false;
  }

  if (!CanAppendToListPack(field, value)) {
    ConvertListPackToDict(Size() + 1);
    SetDict(field, value);
    return true;
  }
  auto field_copy = field;
  auto value_copy = value;
  const std::vector<in_memory::ListPack::ListPackEntry> entries = {
      {&field_copy, 0},
      {&value_copy, 0},
  };
  return listpack_->BatchAppend(entries);
}

bool Hash::SetDict(const std::string& field, const std::string& value) {
  assert(encoding_ == Encoding::kDict);
  if (dict_ == nullptr) {
    dict_ = in_memory::Dict<std::string, std::string>::Create();
  }
  auto* existing = dict_->FindValue(field);
  if (existing != nullptr) {
    *existing = value;
    return false;
  }
  dict_->Set(field, value);
  return true;
}

void Hash::ConvertListPackToDict(size_t capacity) {
  assert(encoding_ == Encoding::kListPack);
  encoding_ = Encoding::kDict;
  dict_ = in_memory::Dict<std::string, std::string>::Create(capacity);
  if (listpack_ == nullptr) {
    return;
  }
  ssize_t field_idx = listpack_->First();
  while (field_idx != -1) {
    const ssize_t value_idx = listpack_->Next(static_cast<size_t>(field_idx));
    if (value_idx < 0) {
      break;
    }
    auto field = listpack_->Get(static_cast<size_t>(field_idx));
    auto value = listpack_->Get(static_cast<size_t>(value_idx));
    if (field.has_value() && value.has_value()) {
      dict_->Set(*field, *value);
    }
    field_idx = listpack_->Next(static_cast<size_t>(value_idx));
  }
  listpack_.reset();
}

std::optional<ssize_t> Hash::FindListPackField(
    const std::string& field) const {
  assert(encoding_ == Encoding::kListPack);
  ssize_t idx = listpack_->First();
  bool is_field = true;
  while (idx != -1) {
    if (is_field) {
      const auto value = listpack_->Get(static_cast<size_t>(idx));
      if (value.has_value() && *value == field) {
        return idx;
      }
    }
    idx = listpack_->Next(static_cast<size_t>(idx));
    is_field = !is_field;
  }
  return std::nullopt;
}

std::vector<Hash::Entry> Hash::ListPackEntries() const {
  std::vector<Entry> entries;
  entries.reserve(Size());
  ssize_t field_idx = listpack_->First();
  while (field_idx != -1) {
    const ssize_t value_idx = listpack_->Next(static_cast<size_t>(field_idx));
    if (value_idx < 0) {
      break;
    }
    auto field = listpack_->Get(static_cast<size_t>(field_idx));
    auto value = listpack_->Get(static_cast<size_t>(value_idx));
    if (field.has_value() && value.has_value()) {
      entries.push_back({std::move(*field), std::move(*value)});
    }
    field_idx = listpack_->Next(static_cast<size_t>(value_idx));
  }
  return entries;
}

std::vector<Hash::Entry> Hash::DictEntries() const {
  std::vector<Entry> entries;
  entries.reserve(Size());
  auto it = in_memory::Dict<std::string, std::string>::Iterator(dict_.get());
  it.SeekToFirst();
  while (it.Valid()) {
    entries.push_back({it.Key(), it.Value()});
    it.Next();
  }
  return entries;
}
}  // namespace redis_simple::hash
