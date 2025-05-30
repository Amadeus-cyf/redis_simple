#include "memory/intset.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace redis_simple {
namespace in_memory {
IntSet::IntSet()
    : contents_(new int[initSize]), length_(0), encoding_(intsetEncoding16) {}
/*
 * Add the value to the intset. Return true if succeeded.
 */
bool IntSet::Add(int64_t value) {
  unsigned int value_encode = ValueEncoding(value);
  if (value_encode > encoding_) {
    UpgradeAndAdd(value);
    return true;
  }
  unsigned int index = 0;
  if (Search(value, &index)) return false;
  Resize(length_ + 1);
  if (index < length_) MoveTail(index, index + 1);
  Set(index, value);
  ++length_;
  return true;
}

/*
 * Return the value at the index. Return true if succeeded.
 */
int64_t IntSet::Get(unsigned int index) const {
  return GetEncoded(index, encoding_);
}

bool IntSet::Find(int64_t value) const {
  EncodingType encoding = ValueEncoding(value);
  return encoding <= encoding_ && Search(value, nullptr);
}

/*
 * Remove the value from the intset. Return true if succeeded.
 */
bool IntSet::Remove(int64_t value) {
  unsigned int index = 0;
  bool exist = Search(value, &index);
  if (!exist) return false;
  MoveTail(index + 1, index);
  Resize(length_ - 1);
  --length_;
  return true;
}

/*
 * Return the maximum element from the intset.
 */
int64_t IntSet::Max() const { return Get(length_ - 1); }

/*
 * Return the minimum element from the intset.
 */
int64_t IntSet::Min() const { return Get(0); }

/*
 * Get encoding type of the value.
 */
IntSet::EncodingType IntSet::ValueEncoding(const int64_t value) const {
  if (value < INT32_MIN || value > INT32_MAX) {
    return intsetEncoding64;
  } else if (value < INT16_MIN || value > INT16_MAX) {
    return intsetEncoding32;
  } else {
    return intsetEncoding16;
  }
}

void IntSet::Resize(unsigned int length) {
  contents_ = static_cast<int*>(std::realloc(contents_, length * encoding_));
}

/*
 * Upgrade the encoding and add the value.
 */
void IntSet::UpgradeAndAdd(int64_t value) {
  EncodingType cur_encode = encoding_;
  encoding_ = ValueEncoding(value);
  unsigned int length = length_;
  Resize(length_ + 1);
  int prepend = value < 0 ? 1 : 0;
  while (length-- > 0) {
    Set(length + prepend, GetEncoded(length, cur_encode));
  }
  if (prepend == 1) {
    Set(0, value);
  } else {
    Set(length_, value);
  }
  ++length_;
}

/*
 * Search for the value and set index to the index of the value if the value is
 * found. return true if the value is found.
 */
bool IntSet::Search(int64_t value, unsigned int* const index) const {
  if (length_ > 0 && value < GetEncoded(0, encoding_)) {
    if (index) *index = 0;
    return false;
  } else if (length_ > 0 && value > GetEncoded(length_ - 1, encoding_)) {
    if (index) *index = length_;
    return false;
  }
  int min_idx = 0, max_idx = length_;
  while (min_idx < max_idx) {
    int mid = min_idx + (max_idx - min_idx) / 2;
    int64_t val = GetEncoded(mid, encoding_);
    if (val == value) {
      if (index) *index = mid;
      return true;
    } else if (val < value) {
      min_idx = mid + 1;
    } else {
      max_idx = mid;
    }
  }
  if (index) *index = min_idx;
  return false;
}

/*
 * Get the value at the index based on the encoding.
 */
int64_t IntSet::GetEncoded(unsigned int index, EncodingType encoding) const {
  if (index >= length_) throw std::out_of_range("index out of bound");
  if (encoding == intsetEncoding64) {
    int64_t v64;
    std::memcpy(&v64, reinterpret_cast<int64_t*>(contents_) + index,
                sizeof(v64));
    return v64;
  } else if (encoding == intsetEncoding32) {
    int32_t v32;
    std::memcpy(&v32, reinterpret_cast<int32_t*>(contents_) + index,
                sizeof(v32));
    return v32;
  } else {
    int16_t v16;
    std::memcpy(&v16, reinterpret_cast<int16_t*>(contents_) + index,
                sizeof(v16));
    return v16;
  }
}

void IntSet::Set(unsigned int index, int64_t value) {
  if (encoding_ == intsetEncoding64) {
    reinterpret_cast<int64_t*>(contents_)[index] = value;
  } else if (encoding_ == intsetEncoding32) {
    reinterpret_cast<int32_t*>(contents_)[index] = value;
  } else {
    reinterpret_cast<int16_t*>(contents_)[index] = value;
  }
}

/*
 * Move all data from one index to another.
 */
void IntSet::MoveTail(unsigned int from, unsigned int to) {
  void *src, *dst;
  unsigned int bytes = length_ - from;
  if (encoding_ == intsetEncoding64) {
    src = reinterpret_cast<int64_t*>(contents_) + from;
    dst = reinterpret_cast<int64_t*>(contents_) + to;
    bytes *= sizeof(int64_t);
  } else if (encoding_ == intsetEncoding32) {
    src = reinterpret_cast<int32_t*>(contents_) + from;
    dst = reinterpret_cast<int32_t*>(contents_) + to;
    bytes *= sizeof(int32_t);
  } else {
    src = reinterpret_cast<int16_t*>(contents_) + from;
    dst = reinterpret_cast<int16_t*>(contents_) + to;
    bytes *= sizeof(int16_t);
  }
  std::memmove(dst, src, bytes);
}
}  // namespace in_memory
}  // namespace redis_simple
