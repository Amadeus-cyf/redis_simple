#include "set.h"

#include <cassert>
#include <optional>

#include "utils/int_utils.h"
#include "utils/string_utils.h"

namespace redis_simple::set {
Set::Set()
    : encoding_(SetEncodingType::kIntSet),
      intset_(nullptr),
      dict_(nullptr),
      listpack_(nullptr) {}

bool Set::Add(const std::string& value) {
  if (encoding_ == SetEncodingType::kIntSet) {
    return IntSetAddAndMaybeConvert(value);
  }
  if (encoding_ == SetEncodingType::kListPack) {
    return ListPackAddAndMaybeConvert(value);
  } else if (encoding_ == SetEncodingType::kDict) {
    return DictAdd(value);
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

bool Set::HasMember(const std::string& value) const {
  if (Size() == 0) {
    return false;
  }
  if (encoding_ == SetEncodingType::kIntSet) {
    int64_t int_val = 0;
    if (!utils::ToInt64(value, &int_val)) {
      return false;
    }
    return intset_->Find(int_val);
  }
  if (encoding_ == SetEncodingType::kListPack) {
    return listpack_->Find(value) != -1;
  } else if (encoding_ == SetEncodingType::kDict) {
    return dict_->Get(value).has_value();
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

std::vector<std::string> Set::ListAllMembers() const {
  if (Size() == 0) {
    return {};
  }
  if (encoding_ == SetEncodingType::kIntSet) {
    return ListIntSetMembers();
  }
  if (encoding_ == SetEncodingType::kListPack) {
    return ListListPackMembers();
  } else if (encoding_ == SetEncodingType::kDict) {
    return ListDictMembers();
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

bool Set::Remove(const std::string& value) {
  if (Size() == 0) {
    return false;
  }
  if (encoding_ == SetEncodingType::kIntSet) {
    int64_t int_val = 0;
    if (utils::ToInt64(value, &int_val)) {
      return intset_->Remove(int_val);
    }
    return false;
  }
  if (encoding_ == SetEncodingType::kListPack) {
    ssize_t idx = listpack_->Find(value);
    if (idx < 0) return false;
    listpack_->Delete(idx);
    return true;
  } else if (encoding_ == SetEncodingType::kDict) {
    return dict_->Delete(value);
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

size_t Set::Size() const {
  switch (encoding_) {
    case SetEncodingType::kIntSet:
      return intset_ ? intset_->Size() : 0;
    case SetEncodingType::kListPack:
      return listpack_ ? listpack_->Size() : 0;
    case SetEncodingType::kDict:
      return dict_ ? dict_->Size() : 0;
    default:
      throw std::invalid_argument("unknown encoding type");
  }
}

Set::Encoding Set::GetEncoding() const {
  switch (encoding_) {
    case SetEncodingType::kIntSet:
      return Encoding::kIntSet;
    case SetEncodingType::kListPack:
      return Encoding::kListPack;
    case SetEncodingType::kDict:
      return Encoding::kDict;
    default:
      throw std::invalid_argument("unknown encoding type");
  }
}

bool Set::IntSetAddAndMaybeConvert(const std::string& value) {
  int64_t int_val = 0;
  if (utils::ToInt64(value, &int_val)) {
    if (!intset_) {
      intset_ = std::make_unique<in_memory::IntSet>();
    }
    bool success = intset_->Add(int_val);
    if (success) {
      MaybeConvertIntsetToDict();
    }
    return success;
  }
  if (!MaybeConvertIntSetToListPack(value)) {
    ConvertIntSetToDict((intset_ ? intset_->Size() : 0) + 1);
    dict_->Set(value, nullptr);
  }
  return true;
}

bool Set::ListPackAddAndMaybeConvert(const std::string& value) {
  size_t len = value.size();
  if (listpack_->Find(value) != -1) {
    return false;
  }
  if (listpack_->Size() < kListPackMaxEntries &&
      len <= kListPackElementMaxLength &&
      in_memory::ListPack::SafeToAdd(listpack_.get(), len)) {
    return listpack_->Append(value);
  }
  ConvertListPackToDict(listpack_->Size() + 1);
  if (dict_->Get(value).has_value()) return false;
  dict_->Set(value, nullptr);
  return true;
}

bool Set::DictAdd(const std::string& value) {
  if (!dict_) {
    dict_ = in_memory::Dict<std::string, nullptr_t>::Init();
  }
  if (dict_->Get(value).has_value()) {
    return false;
  }
  dict_->Set(value, nullptr);
  return true;
}

void Set::MaybeConvertIntsetToDict() {
  assert(encoding_ == SetEncodingType::kIntSet);
  if (intset_ && intset_->Size() > kIntSetMaxEntries) {
    ConvertIntSetToDict(intset_->Size());
  }
}

void Set::ConvertIntSetToDict(size_t capacity) {
  assert(encoding_ == SetEncodingType::kIntSet);
  encoding_ = SetEncodingType::kDict;
  dict_ = in_memory::Dict<std::string, nullptr_t>::Init(capacity);
  if (!intset_) {
    return;
  }
  for (unsigned int i = 0; i < intset_->Size(); ++i) {
    int64_t value = intset_->Get(i);
    dict_->Set(std::to_string(value), nullptr);
  }
  intset_.reset();
}

bool Set::MaybeConvertIntSetToListPack(const std::string& val) {
  if (encoding_ != SetEncodingType::kIntSet) {
    return false;
  }
  size_t len = val.size();
  size_t max_integer_length = 0;
  size_t estimated_bytes = 0;
  int64_t estimated_integer = 0;
  if (intset_) {
    int64_t max_integer = intset_->Max();
    int64_t min_integer = intset_->Min();
    size_t max_integer_digits = utils::Digits10(max_integer);
    size_t min_integer_digits = utils::Digits10(min_integer);
    max_integer_length = std::max(max_integer_digits, min_integer_digits);
    estimated_integer =
        max_integer_digits > min_integer_digits ? max_integer : min_integer;
    estimated_bytes =
        in_memory::ListPack::EstimateBytes(estimated_integer, intset_->Size());
  }
  if (!intset_ ||
      (intset_->Size() < kListPackMaxEntries &&
       len <= kListPackElementMaxLength &&
       max_integer_length <= kListPackElementMaxLength &&
       in_memory::ListPack::SafeToAdd(nullptr, estimated_bytes + len))) {
    ConvertIntSetToListPack(val);
    return true;
  }
  return false;
}

void Set::ConvertIntSetToListPack(const std::string& val) {
  assert(encoding_ == SetEncodingType::kIntSet);
  encoding_ = SetEncodingType::kListPack;
  listpack_ = std::make_unique<in_memory::ListPack>();
  if (intset_) {
    for (unsigned int i = 0; i < intset_->Size(); ++i) {
      int64_t value = intset_->Get(i);
      listpack_->Append(value);
    }
  }
  listpack_->Append(val);
  intset_.reset();
}

void Set::ConvertListPackToDict(size_t capacity) {
  assert(encoding_ == SetEncodingType::kListPack);
  encoding_ = SetEncodingType::kDict;
  dict_ = in_memory::Dict<std::string, nullptr_t>::Init(capacity);
  if (!listpack_) {
    return;
  }
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    const auto string_result = listpack_->Get(idx);
    if (string_result.has_value()) {
      dict_->Set(*string_result, nullptr);
    }
    idx = listpack_->Next(idx);
  }
  listpack_.reset();
}

std::vector<std::string> Set::ListIntSetMembers() const {
  std::vector<std::string> members;
  auto it = in_memory::IntSet::Iterator(intset_.get());
  it.SeekToFirst();
  while (it.Valid()) {
    members.push_back(std::to_string(it.Value()));
    it.Next();
  }
  return members;
}

std::vector<std::string> Set::ListListPackMembers() const {
  std::vector<std::string> members;
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    const auto string_result = listpack_->Get(idx);
    if (string_result.has_value()) {
      members.push_back(*string_result);
    }
    idx = listpack_->Next(idx);
  }
  return members;
}

std::vector<std::string> Set::ListDictMembers() const {
  std::vector<std::string> members;
  auto it = in_memory::Dict<std::string, nullptr_t>::Iterator(dict_.get());
  it.SeekToFirst();
  while (it.Valid()) {
    members.push_back(it.Key());
    it.Next();
  }
  return members;
}
}  // namespace redis_simple::set
