#pragma once

#include <sys/types.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace redis_simple::in_memory {
// Compact Redis-style sequential encoding. See:
// https://github.com/antirez/listpack/blob/master/listpack.md
class ListPack {
 public:
  struct ListPackEntry {
    std::string* const str;
    // str is null when the entry stores an integer.
    int64_t sval;
  };
  // Header: 32-bit total bytes followed by 16-bit element count.
  static constexpr int kListPackHeaderSize = 6;
  ListPack();
  explicit ListPack(size_t capacity);
  ListPack(const ListPack&) = delete;
  ListPack& operator=(const ListPack&) = delete;
  unsigned char* Get(size_t idx, size_t* const len) const;
  std::optional<std::string> Get(size_t idx) const;
  std::optional<int64_t> IntegerAt(size_t idx) const;
  ssize_t Find(const std::string& val) const;
  ssize_t FindAndSkip(const std::string& val, size_t skip) const;
  bool Append(const std::string& element_string);
  bool Append(int64_t element_integer);
  bool Prepend(const std::string& element_string);
  bool Prepend(int64_t element_integer);
  bool Insert(size_t idx, const std::string& element_string);
  bool Insert(size_t idx, int64_t element_integer);
  bool Replace(size_t idx, const std::string& element_string);
  bool Replace(size_t idx, int64_t element_integer);
  bool BatchAppend(const std::vector<ListPackEntry>& entries);
  bool BatchPrepend(const std::vector<ListPackEntry>& entries);
  bool BatchInsert(size_t idx, const std::vector<ListPackEntry>& entries);
  void Delete(size_t idx);
  ssize_t First() const;
  ssize_t Last() const;
  ssize_t Next(size_t idx) const;
  ssize_t Prev(size_t idx) const;
  uint32_t TotalBytes() const;
  size_t Size() const;
  static size_t EstimateBytes(int64_t lval, size_t repeat);
  static bool SafeToAdd(const ListPack* const lp, size_t bytes);
  ~ListPack() = default;

 private:
  // Enough space for INT64_MIN plus a null terminator.
  static constexpr int kListPackIntBufSize = 21;
  static constexpr uint16_t kListPackNumEleUnknown = UINT16_MAX;
  static constexpr int kUint7BitIntMax = 127;
  static constexpr int kInt24BitIntMax = (1 << 23) - 1;
  static constexpr int kInt24BitIntMin = -(1 << 23);
  // Keep reallocations below the unsigned 32-bit total-bytes limit.
  static constexpr uint32_t kListPackMaxSafetySize = 1 << 30;
  static constexpr int64_t kListPackEof = 0xff;
  enum class EncodingGeneralType {
    kInteger = 0,
    kString = 1,
  };
  enum EncodingType {
    k7BitUnsignedInteger = 0,
    k6BitString = 0x80,
    k13BitInteger = 0xc0,
    k12BitString = 0xe0,
    k32BitString = 0xf0,
    k16BitInteger = 0xf1,
    k24BitInteger = 0xf2,
    k32BitInteger = 0xf3,
    k64BitInteger = 0xf4,
  };
  enum EncodingTypeMask {
    kType7BitUintMask = 0x80,
    kType6BitStrMask = 0xc0,
    kType13BitIntMask = 0xe0,
    kType12BitStrMask = 0xf0,
    kType16BitIntMask = 0xff,
    kType24BitIntMask = 0xff,
    kType32BitIntMask = 0xff,
    kType32BitStrMask = 0xff,
    kType64BitIntMask = 0xff,
  };
  enum class Position {
    kInsertAfter = 0,
    kInsertBefore = 1,
    kReplace = 2,
  };
  enum BacklenThreshold : uint64_t {
    kSize1ByteBacklenMax = (1ULL << 7) - 1,
    kSize2BytesBacklenMax = (1ULL << 14) - 1,
    kSize3BytesBacklenMax = (1ULL << 21) - 1,
    kSize4BytesBacklenMax = (1ULL << 28) - 1,
    kSize5BytesBacklenMax = (1ULL << 35) - 1,
  };
  struct Encoding {
    std::string* str;
    int64_t sval;
    EncodingGeneralType encoding_type;
    size_t backlen_bytes;
  };
  unsigned char* StringAt(size_t idx, size_t* const len,
                          EncodingType encoding_type) const;
  unsigned char* IntegerAt(size_t idx, unsigned char* dst, size_t* const len,
                           int64_t* val, EncodingType encoding_type) const;
  bool Insert(size_t idx, ListPack::Position where,
              const std::string* element_string,
              const int64_t* element_integer);
  bool BatchInsert(size_t idx, ListPack::Position where,
                   const std::vector<ListPackEntry>& entries);
  void SetTotalBytes(uint32_t listpack_bytes);
  uint16_t ElementCount() const;
  void SetNumOfElements(uint16_t num_of_elements) const;
  size_t Skip(size_t idx) const;
  EncodingType EncodingAt(size_t idx) const;
  size_t Backlen(size_t idx) const;
  static uint8_t BacklenBytes(size_t backlen);
  static size_t EncodeString(unsigned char* const buf, const std::string* ele);
  static size_t EncodeInteger(unsigned char* const buf, int64_t v);
  static void EncodeBacklen(unsigned char* const buf, size_t backlen);
  size_t DecodeBacklen(size_t idx) const;
  size_t DecodeStringLength(size_t idx) const;
  static bool IsString(EncodingType encoding_type);
  void Realloc(size_t bytes);
  std::unique_ptr<unsigned char[]> lp_;
  mutable unsigned char int_buf_[kListPackIntBufSize]{};
};
}  // namespace redis_simple::in_memory
