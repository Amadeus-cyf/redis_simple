#include "set.h"

#include <cassert>

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
  if (encoding_ == SetEncodingType::setEncodingIntSet &&
      utils::ToInt64(value, nullptr)) {
    if (!intset_) intset_ = std::make_unique<in_memory::IntSet>();
    int64_t int64_value = std::stoll(value);
    bool success = intset_->Add(int64_value);
    if (success) MaybeConvertIntset();
    return success;
  }
  if (!dict_) {
    dict_ = std::unique_ptr<in_memory::Dict<std::string, nullptr_t>>(
        in_memory::Dict<std::string, nullptr_t>::Init());
    ConvertIntSetToDict();
  }
  if (dict_->Get(value).has_value()) return false;
  dict_->Set(value, nullptr);
  return true;
}

/*
 * Return true if the value is in the set.
 */
bool Set::HasMember(const std::string& value) const {
  if (Size() == 0) return false;
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    if (!utils::ToInt64(value, nullptr)) return false;
    int64_t int64_value = std::stoll(value);
    return intset_->Find(int64_value);
  }
  return dict_->Get(value).has_value();
}

/*
 * List all members in the set.
 */
std::vector<std::string> Set::ListAllMembers() const {
  if (Size() == 0) return {};
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    return ListIntSetMembers();
  }
  return ListDictMembers();
}

/*
 * Remove the value from the set. Return true if succeeded.
 */
bool Set::Remove(const std::string& value) {
  if (Size() == 0) return false;
  if (encoding_ == SetEncodingType::setEncodingIntSet &&
      utils::ToInt64(value, nullptr)) {
    if (!utils::ToInt64(value, nullptr)) return false;
    int64_t int64_value = std::stoll(value);
    return intset_->Remove(int64_value);
  }
  return dict_->Delete(value);
}

/*
 * Return number of elements in the set.
 */
size_t Set::Size() const {
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    return intset_ ? intset_->Size() : 0;
  }
  return dict_ ? dict_->Size() : 0;
}

/*
 * Convert set encoding from intset to dict if inset size exceed the max entries
 */
void Set::MaybeConvertIntset() {
  assert(encoding_ == SetEncodingType::setEncodingIntSet);
  if (intset_ && intset_->Size() > IntSetMaxEntries) ConvertIntSetToDict();
}

/*
 * Convert set encoding from intset to dict and migrate all data.
 */
void Set::ConvertIntSetToDict() {
  assert(encoding_ == SetEncodingType::setEncodingIntSet);
  encoding_ = SetEncodingType::setEncodingDict;
  if (!intset_) return;
  for (unsigned int i = 0; i < intset_->Size(); ++i) {
    int64_t value = intset_->Get(i);
    dict_->Set(std::to_string(value), nullptr);
  }
  intset_.reset();
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
