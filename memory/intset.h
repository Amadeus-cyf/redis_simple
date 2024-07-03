#pragma once

#include <unistd.h>

namespace redis_simple {
namespace in_memory {
class IntSet {
 public:
  IntSet()
      : contents_(new int[initSize]), length_(0), encoding_(intsetEncoding16){};
  bool Add(const int64_t value);
  int64_t Get(const unsigned int index);
  bool Find(const int64_t value);

 private:
  static constexpr unsigned initSize = 1;
  enum EncodingType {
    intsetEncoding16 = sizeof(int16_t),
    intsetEncoding32 = sizeof(int32_t),
    intsetEncoding64 = sizeof(int64_t),
  };
  EncodingType ValueEncoding(const int64_t value);
  void Resize(const unsigned int length_);
  int64_t GetEncoded(const unsigned int index, EncodingType encoding);
  void UpgradeAndAdd(const int64_t value);
  bool Search(const int64_t value, unsigned int* const index);
  void Set(const unsigned int index, const int64_t value);
  void MoveTail(const unsigned int from, const unsigned int to);
  int* contents_;
  unsigned int length_;
  EncodingType encoding_;
};
}  // namespace in_memory
}  // namespace redis_simple
