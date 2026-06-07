#pragma once

#include <unistd.h>

#include <optional>
#include <string>
#include <vector>

namespace redis_simple {
namespace in_memory {
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
  static constexpr int ListPackHeaderSize = 6;
  ListPack();
  explicit ListPack(size_t capacity);
  unsigned char* Get(size_t idx, size_t* const len) const;
  std::optional<std::string> Get(size_t idx) const;
  std::optional<int64_t> GetInteger(size_t idx) const;
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
  uint32_t GetTotalBytes() const;
  size_t Size() const;
  static size_t EstimateBytes(int64_t lval, size_t repeat);
  static bool SafeToAdd(const ListPack* const lp, size_t bytes);
  ~ListPack() {
    delete[] lp_;
    lp_ = nullptr;
  }

 private:
  // Enough space for INT64_MIN plus a null terminator.
  static constexpr int ListPackIntBufSize = 21;
  static constexpr uint16_t ListPackNumEleUnknown = UINT16_MAX;
  static constexpr int Uint7BitIntMax_ = 127;
  static constexpr int Int24BitIntMax = (1 << 23) - 1;
  static constexpr int Int24BitIntMin = -(1 << 23);
  // Keep reallocations below the unsigned 32-bit total-bytes limit.
  static constexpr uint32_t ListPackMaxSafetySize = 1 << 30;
  static constexpr int64_t ListPackEOF = 0xff;
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
    type7BitUIntMask = 0x80,
    type6BitStrMask = 0xc0,
    type13BitIntMask = 0xe0,
    type12BitStrMask = 0xf0,
    type16BitIntMask = 0xff,
    type24BitIntMask = 0xff,
    type32BitIntMask = 0xff,
    type32BitStrMask = 0xff,
    type64bitIntMask = 0xff,
  };
  enum class Position {
    InsertAfter = 0,
    InsertBefore = 1,
    Replace = 2,
  };
  enum BacklenThreshold : uint64_t {
    Size1ByteBacklenMax = (1ULL << 7) - 1,
    Size2BytesBacklenMax = (1ULL << 14) - 1,
    Size3BytesBacklenMax = (1ULL << 21) - 1,
    Size4BytesBacklenMax = (1ULL << 28) - 1,
    Size5BytesBacklenMax = (1ULL << 35) - 1,
  };
  struct Encoding {
    std::string* str;
    int64_t sval;
    EncodingGeneralType encoding_type;
    size_t backlen_bytes;
  };
  unsigned char* GetString(size_t idx, size_t* const len,
                           EncodingType encoding_type) const;
  unsigned char* GetInteger(size_t idx, unsigned char* dst, size_t* const len,
                            int64_t* val, EncodingType encoding_type) const;
  bool Insert(size_t idx, ListPack::Position where,
              const std::string* element_string, int64_t* element_integer);
  bool BatchInsert(size_t idx, ListPack::Position where,
                   const std::vector<ListPackEntry>& entries);
  void SetTotalBytes(uint32_t total_bytes_);
  uint16_t GetNumOfElements() const;
  void SetNumOfElements(uint16_t num_of_elements) const;
  size_t Skip(size_t idx) const;
  EncodingType GetEncodingType(size_t idx) const;
  size_t GetBacklen(size_t idx) const;
  static uint8_t GetBacklenBytes(size_t backlen);
  static size_t EncodeString(unsigned char* const buf, const std::string* s);
  static size_t EncodeInteger(unsigned char* const buf, int64_t ele);
  static void EncodeBacklen(unsigned char* const buf, size_t backlen);
  size_t DecodeBacklen(size_t idx) const;
  size_t DecodeStringLength(size_t idx) const;
  static bool IsString(EncodingType encoding_type);
  void Realloc(size_t size);
  void Free();
  unsigned char* lp_;
};
}  // namespace in_memory
}  // namespace redis_simple
