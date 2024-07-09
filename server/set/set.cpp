#include "set.h"

namespace redis_simple {
namespace set {
/*
 * Add the value to the set. Return true if succeeded.
 */
bool Set::Add(const std::string& value) {
  if (encoding_ == SetEncodingType::setEncodingIntSet && IsInt64(value)) {
    if (!intset_) intset_ = std::make_unique<in_memory::IntSet>();
    int64_t int64_value = std::stoll(value);
    return intset_->Add(int64_value);
  }
  if (!dict_) {
    dict_ = std::unique_ptr<in_memory::Dict<std::string, nullptr_t>>(
        in_memory::Dict<std::string, nullptr_t>::Init());
    ConvertIntSetToDict();
  }
  encoding_ = SetEncodingType::setEncodingDict;
  if (dict_->Get(value).has_value()) return false;
  dict_->Set(value, nullptr);
  return true;
}

/*
 * Return true if the value is in the set.
 */
bool Set::Contains(const std::string& value) {
  if (Size() == 0) return false;
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    if (!IsInt64(value)) return false;
    int64_t int64_value = std::stoll(value);
    return intset_->Find(int64_value);
  }
  return dict_->Get(value).has_value();
}

/*
 * Remove the value from the set. Return true if succeeded.
 */
bool Set::Remove(const std::string& value) {
  if (Size() == 0) return false;
  if (encoding_ == SetEncodingType::setEncodingIntSet && IsInt64(value)) {
    if (!IsInt64(value)) return false;
    int64_t int64_value = std::stoll(value);
    intset_->Remove(int64_value);
  }
  return dict_->Delete(value);
}

/*
 * Return number of elements in the set.
 */
size_t Set::Size() {
  if (encoding_ == SetEncodingType::setEncodingIntSet) {
    return intset_ ? intset_->Size() : 0;
  }
  return dict_ ? dict_->Size() : 0;
}

/*
 * Return true if the value could be converted to int64
 */
bool Set::IsInt64(const std::string& value) {
  try {
    std::stoll(value);
    return true;
  } catch (const std::exception& e) {
    return false;
  }
}

/*
 * Convert set encoding from intset to dict and migrate all data.
 */
void Set::ConvertIntSetToDict() {
  for (unsigned int i = 0; i < intset_->Size(); ++i) {
    int64_t value = intset_->Get(i);
    dict_->Set(std::to_string(value), nullptr);
  }
  intset_.reset();
}
}  // namespace set
}  // namespace redis_simple
