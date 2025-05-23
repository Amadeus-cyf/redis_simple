#include "set.h"

#include <cassert>
#include <optional>

#include "utils/int_utils.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace set {
Set::Set()
    : encoding_(SetEncodingType::IntSet),
      intset_(nullptr),
      dict_(nullptr),
      listpack_(nullptr) {}

/*
 * Add the value to the set. Return true if succeeded.
 */
bool Set::Add(const std::string& value) {
  int64_t int_val = 0;
  if (encoding_ == SetEncodingType::IntSet) {
    return IntSetAddAndMaybeConvert(value);
  } else if (encoding_ == SetEncodingType::ListPack) {
    return ListPackAddAndMaybeConvert(value);
  } else if (encoding_ == SetEncodingType::Dict) {
    return DictAdd(value);
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

/*
 * Return true if the value is in the set.
 */
bool Set::HasMember(const std::string& value) const {
  if (Size() == 0) return false;
  if (encoding_ == SetEncodingType::IntSet) {
    int64_t int_val = 0;
    if (!utils::ToInt64(value, &int_val)) return false;
    return intset_->Find(int_val);
  } else if (encoding_ == SetEncodingType::ListPack) {
    return listpack_->Find(value) != -1;
  } else if (encoding_ == SetEncodingType::Dict) {
    return dict_->Get(value).has_value();
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

/*
 * List all members in the set.
 */
std::vector<std::string> Set::ListAllMembers() const {
  if (Size() == 0) return {};
  if (encoding_ == SetEncodingType::IntSet) {
    return ListIntSetMembers();
  } else if (encoding_ == SetEncodingType::ListPack) {
    return ListListPackMembers();
  } else if (encoding_ == SetEncodingType::Dict) {
    return ListDictMembers();
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

/*
 * Remove the value from the set. Return true if succeeded.
 */
bool Set::Remove(const std::string& value) {
  if (Size() == 0) return false;
  if (encoding_ == SetEncodingType::IntSet) {
    int64_t int_val = 0;
    if (utils::ToInt64(value, &int_val)) {
      return intset_->Remove(int_val);
    }
    return false;
  } else if (encoding_ == SetEncodingType::ListPack) {
    ssize_t idx = listpack_->Find(value);
    if (idx < 0) return false;
    listpack_->Delete(idx);
    return true;
  } else if (encoding_ == SetEncodingType::Dict) {
    return dict_->Delete(value);
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

/*
 * Return number of elements in the set.
 */
size_t Set::Size() const {
  switch (encoding_) {
    case SetEncodingType::IntSet:
      return intset_ ? intset_->Size() : 0;
    case SetEncodingType::ListPack:
      return listpack_ ? listpack_->Size() : 0;
    case SetEncodingType::Dict:
      return listpack_ ? listpack_->Size() : 0;
    default:
      throw std::invalid_argument("unknown encoding type");
  }
}

/*
 * Add the element to the set with the encoding type as intset. Convert the
 * intset to listpack/dict if needed.
 */
bool Set::IntSetAddAndMaybeConvert(const std::string& value) {
  int64_t int_val = 0;
  if (utils::ToInt64(value, &int_val)) {
    if (!intset_) intset_ = std::make_unique<in_memory::IntSet>();
    bool success = intset_->Add(int_val);
    if (success) MaybeConvertIntsetToDict();
    return success;
  } else if (!MaybeConvertIntSetToListPack(value)) {
    // Convert intset to dict if cannot convert it to listpack.
    ConvertIntSetToDict((intset_ ? intset_->Size() : 0) + 1);
    dict_->Set(value, nullptr);
  }
  return true;
}

/*
 * Add the element to the set with the encoding type as listpack. Convert the
 * intset to dict if needed.
 */
bool Set::ListPackAddAndMaybeConvert(const std::string& value) {
  size_t len = value.size();
  if (listpack_->Find(value) != -1) return false;
  if (listpack_->Size() < ListPackMaxEntries &&
      len <= ListPackElementMaxLength &&
      in_memory::ListPack::SafeToAdd(listpack_.get(), len)) {
    return listpack_->Append(value);
  } else {
    ConvertListPackToDict(value);
    if (dict_->Get(value).has_value()) return false;
    dict_->Set(value, nullptr);
    return true;
  }
}

/*
 * Add the element to the set with the encoding type as dict.
 */
bool Set::DictAdd(const std::string& value) {
  if (!dict_) dict_ = in_memory::Dict<std::string, nullptr_t>::Init();
  if (dict_->Get(value).has_value()) return false;
  dict_->Set(value, nullptr);
  return true;
}

/*
 * Convert set encoding from intset to dict if inset size exceed the max
 * entries. `val` is non-null if there is a new string element inserted.
 */
void Set::MaybeConvertIntsetToDict() {
  assert(encoding_ == SetEncodingType::IntSet);
  if (intset_ && intset_->Size() > IntSetMaxEntries)
    ConvertIntSetToDict(intset_->Size());
}

/*
 * Convert set encoding from intset to dict with given capacity and migrate all
 * data.
 */
void Set::ConvertIntSetToDict(size_t capacity) {
  assert(encoding_ == SetEncodingType::IntSet);
  encoding_ = SetEncodingType::Dict;
  dict_ = in_memory::Dict<std::string, nullptr_t>::Init(capacity);
  // Migrate all elements from intset to dict.
  if (!intset_) return;
  for (unsigned int i = 0; i < intset_->Size(); ++i) {
    int64_t value = intset_->Get(i);
    dict_->Set(std::to_string(value), nullptr);
  }
  intset_.reset();
}

/*
 * Try to convert an intset to the listpack with a string element inserted.
 */
bool Set::MaybeConvertIntSetToListPack(const std::string& val) {
  if (encoding_ != SetEncodingType::IntSet) return false;
  size_t len = val.size(), int_maxlen = 0, est_bytes = 0;
  int64_t est_int = 0;
  if (intset_) {
    int64_t maxint = intset_->Max();
    int64_t minint = intset_->Min();
    size_t maxint_len = utils::Digits10(maxint);
    size_t minint_len = utils::Digits10(minint);
    int_maxlen = std::max(maxint_len, minint_len);
    // Take the integer with larger length for estimation.
    est_int = maxint_len > minint_len ? maxint : minint;
    // Calculate estimate total bytes.
    est_bytes = in_memory::ListPack::EstimateBytes(est_int, intset_->Size());
  }
  if (!intset_ || (intset_->Size() < ListPackMaxEntries &&
                   len <= ListPackElementMaxLength &&
                   int_maxlen <= ListPackElementMaxLength &&
                   in_memory::ListPack::SafeToAdd(nullptr, est_bytes + len))) {
    ConvertIntSetToListPack(val);
    return true;
  }
  return false;
}

/*
 * Convert intset to listpack with a string element inserted.
 */
void Set::ConvertIntSetToListPack(const std::string& val) {
  assert(encoding_ == SetEncodingType::IntSet);
  encoding_ = SetEncodingType::ListPack;
  listpack_ = std::make_unique<in_memory::ListPack>();
  if (!intset_) return;
  for (unsigned int i = 0; i < intset_->Size(); ++i) {
    int64_t value = intset_->Get(i);
    listpack_->Append(value);
  }
  listpack_->Append(val);
  intset_.reset();
}

/*
 * Convert listpack to dict with a string element inserted.
 */
void Set::ConvertListPackToDict(const std::string& val) {
  assert(encoding_ == SetEncodingType::ListPack &&
         listpack_->Size() >= ListPackMaxEntries);
  encoding_ = SetEncodingType::Dict;
  dict_ = in_memory::Dict<std::string, nullptr_t>::Init(listpack_->Size() + 1);
  if (!listpack_) return;
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    const std::optional<std::string>& opt_str = listpack_->Get(idx);
    if (opt_str.has_value()) dict_->Set(opt_str.value(), nullptr);
    idx = listpack_->Next(idx);
  }
  dict_->Set(val, nullptr);
}

/*
 * List all members in the set for intset encoding type.
 */
std::vector<std::string> Set::ListIntSetMembers() const {
  std::vector<std::string> members;
  in_memory::IntSet::Iterator it(intset_.get());
  it.SeekToFirst();
  while (it.Valid()) {
    members.push_back(std::to_string(it.Value()));
    it.Next();
  }
  return members;
}

/*
 * List all members in the set for listpack encoding type.
 */
std::vector<std::string> Set::ListListPackMembers() const {
  std::vector<std::string> members;
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    size_t len = 0;
    const std::optional<std::string>& opt_str = listpack_->Get(idx);
    if (opt_str.has_value()) members.push_back(opt_str.value());
    idx = listpack_->Next(idx);
  }
  return members;
}

/*
 * List all members in the set for dict encoding type.
 */
std::vector<std::string> Set::ListDictMembers() const {
  std::vector<std::string> members;
  in_memory::Dict<std::string, nullptr_t>::Iterator it(dict_.get());
  it.SeekToFirst();
  while (it.Valid()) {
    members.push_back(it.Key());
    it.Next();
  }
  return members;
}
}  // namespace set
}  // namespace redis_simple
