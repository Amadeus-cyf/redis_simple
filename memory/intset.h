#pragma once

#include <unistd.h>

namespace redis_simple {
namespace in_memory {
/*
 * An in memory set storing integers in ascending order.
 */
class IntSet {
 public:
  class Iterator;
  IntSet();
  bool Add(const int64_t value);
  int64_t Get(unsigned int index) const;
  bool Find(int64_t value) const;
  bool Remove(int64_t value);
  unsigned int Size() const { return length_; }
  ~IntSet() {
    delete[] contents_;
    contents_ = nullptr;
  }

 private:
  static constexpr unsigned initSize = 1;
  enum EncodingType {
    intsetEncoding16 = sizeof(int16_t),
    intsetEncoding32 = sizeof(int32_t),
    intsetEncoding64 = sizeof(int64_t),
  };
  EncodingType ValueEncoding(const int64_t value) const;
  void Resize(unsigned int length_);
  int64_t GetEncoded(unsigned int index, EncodingType encoding) const;
  void UpgradeAndAdd(int64_t value);
  bool Search(int64_t value, unsigned int* const index) const;
  void Set(unsigned int index, const int64_t value);
  void MoveTail(unsigned int from, unsigned int to);
  int* contents_;
  unsigned int length_;
  EncodingType encoding_;
};

class IntSet::Iterator {
 public:
  explicit Iterator(const IntSet* intset) : intset_(intset), idx_(0) {}
  bool operator==(const Iterator& it) {
    return intset_ == it.intset_ && idx_ == it.idx_;
  }
  bool operator!=(const Iterator& it) { return !((*this) == it); }
  /* Return true if the iterator is positioned at a valid element */
  bool Valid() const { return idx_ >= 0 && idx_ < intset_->Size(); }
  /* Position at the first element */
  void SeekToFirst() { idx_ = 0; }
  /* Position at the last element */
  void SeekToLast() { idx_ = intset_->Size() - 1; }
  /* Advance to the next element */
  void Next() { ++idx_; }
  /* Advance to the previous element */
  void Prev() { --idx_; }
  int64_t Value() { return intset_->Get(idx_); }

 private:
  const IntSet* intset_;
  size_t idx_;
};
}  // namespace in_memory
}  // namespace redis_simple
