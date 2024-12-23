#include "set.h"

#include <cassert>

#include "utils/int_utils.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace set {
Set::Set()
    : encoding_(SetEncodingType::setEncodingIntSet),
      intset_(nullptr),
      dict_(nullptr) {}

/*
 * Add the value to the set. Return true if succeeded.
 */
bool Set::Add(const std::string& value) {
  int64_t int_val = 0;
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    if (utils::ToInt64(value, &int_val)) {
      if (!intset_) intset_ = std::make_unique<in_memory::IntSet>();
      bool success = intset_->Add(int_val);
      if (success) MaybeConvertIntsetToDict();
      return success;
    } else {
      if (!MaybeConvertIntSetToListPack(value)) {
        ConvertIntSetToDict((intset_ ? intset_->Size() : 0) + 1);
        dict_->Set(value, nullptr);
      }
      return true;
    }
  } else if (encoding_ == SetEncodingType::setEncodingListPack) {
    size_t len = value.size();
    if (listpack_->Find(value) != -1) return false;
    if (listpack_->GetNumOfElements() < ListPackMaxEntries &&
        len <= in_memory::ListPack::ListPackElementMaxLength &&
        in_memory::ListPack::SafeToAdd(listpack_.get(), len)) {
      return listpack_->Append(value);
    } else {
      ConvertListPackToDict(value);
      if (dict_->Get(value).has_value()) return false;
      dict_->Set(value, nullptr);
      return true;
    }
  } else if (encoding_ == SetEncodingType::setEncodingDict) {
    if (!dict_) dict_ = in_memory::Dict<std::string, nullptr_t>::Init();
    if (dict_->Get(value).has_value()) return false;
    dict_->Set(value, nullptr);
    return true;
  } else {
    throw std::invalid_argument("unknown encoding type");
  }
}

/*
 * Return true if the value is in the set.
 */
bool Set::HasMember(const std::string& value) const {
  if (Size() == 0) return false;
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    int64_t int_val = 0;
    if (!utils::ToInt64(value, &int_val)) return false;
    return intset_->Find(int_val);
  } else if (encoding_ == SetEncodingType::setEncodingListPack) {
    return listpack_->Find(value) != -1;
  } else if (encoding_ == SetEncodingType::setEncodingDict) {
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
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    return ListIntSetMembers();
  } else if (encoding_ == SetEncodingType::setEncodingListPack) {
    return ListListPackMembers();
  } else if (encoding_ == SetEncodingType::setEncodingDict) {
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
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    int64_t int_val = 0;
    if (utils::ToInt64(value, &int_val)) {
      return intset_->Remove(int_val);
    }
    return false;
  } else if (encoding_ == SetEncodingType::setEncodingListPack) {
    ssize_t idx = listpack_->Find(value);
    if (idx < 0) return false;
    listpack_->Delete(idx);
    return true;
  } else if (encoding_ == SetEncodingType::setEncodingDict) {
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
    case SetEncodingType::setEncodingIntSet:
      return intset_ ? intset_->Size() : 0;
    case SetEncodingType::setEncodingListPack:
      return listpack_ ? listpack_->GetNumOfElements() : 0;
    case SetEncodingType::setEncodingDict:
      return listpack_ ? listpack_->GetNumOfElements() : 0;
    default:
      throw std::invalid_argument("unknown encoding type");
  }
}

/*
 * Convert set encoding from intset to dict if inset size exceed the max
 * entries. `val` is non-null if there is a new string element inserted.
 */
void Set::MaybeConvertIntsetToDict() {
  assert(encoding_ == SetEncodingType::setEncodingIntSet);
  if (intset_ && intset_->Size() > IntSetMaxEntries)
    ConvertIntSetToDict(intset_->Size());
}

/*
 * Convert set encoding from intset to dict with given capacity and migrate all
 * data.
 */
void Set::ConvertIntSetToDict(size_t capacity) {
  assert(encoding_ == SetEncodingType::setEncodingIntSet);
  encoding_ = SetEncodingType::setEncodingDict;
  dict_ = in_memory::Dict<std::string, nullptr_t>::Init(capacity);
  /* migrate all elements from intset to dict */
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
  if (encoding_ != SetEncodingType::setEncodingIntSet) return false;
  size_t len = val.size(), int_maxlen = 0, est_bytes = 0;
  int64_t est_int = 0;
  if (intset_) {
    int64_t maxint = intset_->Max();
    int64_t minint = intset_->Min();
    size_t maxint_len = utils::Digits10(maxint);
    size_t minint_len = utils::Digits10(minint);
    int_maxlen = std::max(maxint_len, minint_len);
    /* take the integer with larger length for estimation */
    est_int = maxint_len > minint_len ? maxint : minint;
    /* calculate estimate total bytes */
    est_bytes = in_memory::ListPack::EstimateBytes(est_int, intset_->Size());
  }
  if (!intset_ ||
      (intset_->Size() < ListPackMaxEntries &&
       len <= in_memory::ListPack::ListPackElementMaxLength &&
       int_maxlen <= in_memory::ListPack::ListPackElementMaxLength &&
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
  assert(encoding_ == SetEncodingType::setEncodingIntSet);
  encoding_ = SetEncodingType::setEncodingListPack;
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
  assert(encoding_ == SetEncodingType::setEncodingListPack &&
         listpack_->GetNumOfElements() >= ListPackMaxEntries);
  encoding_ = SetEncodingType::setEncodingDict;
  dict_ = in_memory::Dict<std::string, nullptr_t>::Init(
      listpack_->GetNumOfElements() + 1);
  if (!listpack_) return;
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    size_t len = 0;
    unsigned char* buf = listpack_->Get(idx, &len);
    dict_->Set(std::string(reinterpret_cast<char*>(buf), len), nullptr);
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
    unsigned char* buf = listpack_->Get(idx, &len);
    members.push_back(std::string(reinterpret_cast<char*>(buf), len));
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
