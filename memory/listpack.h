#pragma once

#include <unistd.h>

#include <optional>
#include <string>
#include <vector>

namespace redis_simple {
namespace in_memory {
class ListPack {
 public:
  struct ListPackEntry {
    std::string* const str;
    /* if list pack has an integer value, str should be null */
    int64_t sval;
  };
  /* listpack header size. 32 bit total length + 16 bit number of elements */
  static constexpr int ListPackHeaderSize = 6;
  static constexpr size_t ListPackElementMaxLength = 1UL << 32;
  ListPack();
  explicit ListPack(size_t capacity);
  unsigned char* Get(size_t idx, size_t* const len) const;
  std::optional<std::string> Get(size_t idx) const;
  std::optional<int64_t> GetInteger(size_t idx) const;
  ssize_t Find(const std::string& val) const;
  ssize_t FindAndSkip(const std::string& val, size_t skip) const;
  bool Append(const std::string& elestr);
  bool Append(int64_t eleint);
  bool Prepend(const std::string& elestr);
  bool Prepend(int64_t eleint);
  bool Insert(size_t idx, const std::string& elestr);
  bool Insert(size_t idx, int64_t eleint);
  bool Replace(size_t idx, const std::string& elestr);
  bool Replace(size_t idx, int64_t eleint);
  bool BatchAppend(const std::vector<ListPackEntry>& entries);
  bool BatchPrepend(const std::vector<ListPackEntry>& entries);
  bool BatchInsert(size_t idx, const std::vector<ListPackEntry>& entries);
  void Delete(size_t idx);
  ssize_t First() const;
  ssize_t Last() const;
  ssize_t Next(size_t idx) const;
  ssize_t Prev(size_t idx) const;
  uint32_t GetTotalBytes() const;
  uint16_t GetNumOfElements() const;
  static size_t EstimateBytes(int64_t lval, size_t repetive);
  static bool SafeToAdd(const ListPack* const lp, size_t bytes);
  ~ListPack() {
    delete[] lp_;
    lp_ = nullptr;
  }

 private:
  /* 20 digits of -2^63 + 1 digit of the null term = 21 */
  static constexpr int ListPackIntBufSize = 21;
  static constexpr int Uint7BitIntMax_ = 127;
  static constexpr int Int24BitIntMax = (1 << 23) - 1;
  static constexpr int Int24BitIntMin = -(1 << 23);
  /* max safety bytes of the listpack */
  static constexpr uint32_t ListPackMaxSafetySize = 1 << 30;
  /* the end of the listpack */
  static constexpr int64_t ListPackEOF = 0xff;
  enum class EncodingGeneralType {
    typeInt = 0,
    typeStr = 1,
  };
  enum EncodingType {
    type7BitUInt = 0,
    type6BitStr = 0x80,
    type13BitInt = 0xc0,
    type12BitStr = 0xe0,
    type32BitStr = 0xf0,
    type16BitInt = 0xf1,
    type24BitInt = 0xf2,
    type32BitInt = 0xf3,
    type64BitInt = 0xf4,
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
  enum BacklenThreshold {
    Size1ByteBacklenMax = (1 << 7) - 1,
    Size2BytesBacklenMax = (1 << 14) - 1,
    Size3BytesBacklenMax = (1 << 21) - 1,
    Size4BytesBacklenMax = (1 << 28) - 1,
    Size5BytesBacklenMax = (1 << 35) - 1,
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
  bool Insert(size_t idx, ListPack::Position where, const std::string* elestr,
              int64_t* eleint);
  bool BatchInsert(size_t idx, ListPack::Position where,
                   const std::vector<ListPackEntry>& entries);
  void SetTotalBytes(uint32_t total_bytes_);
  void SetNumOfElements(uint16_t num_of_elements);
  size_t Skip(size_t idx) const;
  EncodingType GetEncodingType(size_t idx) const;
  size_t GetBacklen(size_t idx) const;
  static uint8_t GetBacklenBytes(size_t backlen);
  static size_t EncodeString(unsigned char* const buf, const std::string* s);
  static size_t EncodeInteger(unsigned char* const buf, int64_t ele);
  static void EncodeBacklen(unsigned char* const buf, size_t backlen);
  size_t DecodeBacklen(size_t idx) const;
  size_t DecodeStringLength(size_t idx) const;
  static bool isString(EncodingType encoding_type);
  unsigned char* Malloc(size_t size);
  void Realloc(size_t size);
  void Free();
  unsigned char* lp_;
};
}  // namespace in_memory
}  // namespace redis_simple
