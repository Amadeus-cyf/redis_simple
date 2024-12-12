#pragma once

#include <unistd.h>

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
  ListPack(size_t capacity);
  unsigned char* Get(size_t idx, size_t* const len);
  int64_t GetInteger(size_t idx);
  bool Append(const std::string& elestr);
  bool AppendInteger(int64_t eleint);
  bool BatchAppend(const std::vector<ListPackEntry*>& entries);
  ssize_t Next(size_t idx);
  uint32_t GetTotalBytes();
  uint16_t GetNumOfElements();

 private:
  /* 20 digits of -2^63 + 1 digit of the null term = 21 */
  static constexpr int ListPackIntBufSize = 21;
  static constexpr int Uint7BitIntMax_ = 127;
  static constexpr int Int24BitIntMax = (1 << 23) - 1;
  static constexpr int Int24BitIntMin = -(1 << 23);
  static constexpr uint32_t ListPackSize = UINT32_MAX;
  enum EncodingGeneralType {
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
    lpEOF = 0xff,
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
  enum Position {
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
                           EncodingType encoding_type);
  unsigned char* GetInteger(size_t idx, unsigned char* dst, size_t* const len,
                            int64_t* val, EncodingType encoding_type);
  bool Insert(size_t idx, ListPack::Position where, const std::string* elestr,
              int64_t* eleint);
  bool BatchInsert(size_t idx, ListPack::Position where,
                   const std::vector<ListPackEntry*>& entries);
  void Delete(size_t idx);
  void SetTotalBytes(uint32_t total_bytes_);
  void SetNumOfElements(uint16_t num_of_elements);
  size_t EncodeString(unsigned char* const buf, const std::string* s);
  size_t EncodeInteger(unsigned char* const buf, int64_t ele);
  void EncodeBacklen(unsigned char* const buf, size_t backlen);
  EncodingType GetEncodingType(size_t idx);
  size_t DecodeStringLength(size_t idx);
  size_t DecodeBacklen(size_t idx);
  uint8_t GetBacklenBytes(size_t backlen);
  bool isString(EncodingType encoding_type) {
    return encoding_type == type6BitStr || encoding_type == type12BitStr ||
           encoding_type == type32BitStr;
  }
  unsigned char* Malloc(size_t size);
  void Realloc(size_t size);
  void Free();
  unsigned char* lp_;
};
}  // namespace in_memory
}  // namespace redis_simple
