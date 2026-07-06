#include "data_types/hash/hash.h"

#include <sys/types.h>

#include <cassert>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace redis_simple::hash {
namespace {
constexpr size_t kListPackMaxEntries = 128;
constexpr size_t kListPackMaxElementLength = 64;

bool CanStoreInListPack(const std::string& field, const std::string& value) {
  return field.size() <= kListPackMaxElementLength &&
         value.size() <= kListPackMaxElementLength;
}

std::optional<size_t> ToListPackIndex(ssize_t index) {
  return index < 0 ? std::nullopt
                   : std::optional<size_t>(static_cast<size_t>(index));
}

std::optional<size_t> NextIndex(const in_memory::ListPack* const listpack,
                                size_t index) {
  return ToListPackIndex(listpack->Next(index));
}
}  // namespace

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
    const auto value_idx = NextIndex(listpack_.get(), *field_idx);
    if (!value_idx.has_value()) {
      return std::nullopt;
    }
    return listpack_->Get(*value_idx);
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
  std::vector<Entry> entries;
  entries.reserve(Size());
  ForEachEntry([&entries](std::string_view field, std::string_view value) {
    entries.push_back({std::string(field), std::string(value)});
    return true;
  });
  return entries;
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
    const auto value_idx = NextIndex(listpack_.get(), *field_idx);
    if (!value_idx.has_value()) {
      return false;
    }
    listpack_->Replace(*value_idx, value);
    return false;
  }

  if (!CanAppendToListPack(field, value)) {
    ConvertListPackToDict(Size() + 1);
    SetDict(field, value);
    return true;
  }
  const std::vector<in_memory::ListPack::ListPackEntry> entries = {
      {field, 0, false},
      {value, 0, false},
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
  dict_ = in_memory::Dict<std::string, std::string>::Create(capacity);
  if (listpack_ != nullptr) {
    listpack_->ForEachPair(
        [this](std::string_view field, std::string_view value) {
          dict_->Set(std::string(field), std::string(value));
          return true;
        });
  }
  listpack_.reset();
  encoding_ = Encoding::kDict;
}

std::optional<size_t> Hash::FindListPackField(const std::string& field) const {
  assert(encoding_ == Encoding::kListPack);
  return ToListPackIndex(listpack_->FindAndSkip(field, 1));
}
}  // namespace redis_simple::hash
