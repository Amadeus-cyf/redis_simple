#pragma once

#include <unistd.h>

#include <string>

namespace redis_simple {
namespace in_memory {
class ListPack {
 public:
  ListPack() : total_bytes_(0), num_of_elements_(0){};

 private:
  static constexpr int uint7BitIntMax_ = 127;
  static constexpr int int24BitIntMax = (1 << 23) - 1;
  static constexpr int int24BitIntMin = -(1 << 23);
  enum EncodingGeneralType {
    typeInt = 0,
    typeStr = 1,
  };
  enum EncodingType {
    type7BitUInt = 0,
    type6BitStr = 0x80,
    type13BitInt = 0xC0,
    type12BitStr = 0xE0,
    type32BitStr = 0xF0,
    type16BitInt = 0xF1,
    type24BitInt = 0xF2,
    type32BitInt = 0xF3,
    type64BitInt = 0xF4,
    lpEOF = 0xFF,
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
  unsigned char* Resize(size_t size);
  size_t Next(size_t idx);
  void Insert(size_t idx, ListPack::Position where, const std::string* elestr,
              int64_t* eleint);
  void Delete(size_t idx, const std::string& elestr, int64_t* eleint);
  size_t EncodeString(unsigned char* const buf, const std::string* s);
  size_t EncodeInteger(unsigned char* const buf, int64_t ele);
  void EncodeBacklen(unsigned char* buf, size_t backlen);
  EncodingType GetEncodingType(size_t idx);
  size_t DecodeStrLen(size_t idx);
  size_t DecodeBacklen(size_t idx);
  uint8_t GetBacklenBytes(size_t backlen);
  unsigned char* lp_;
  size_t total_bytes_;
  size_t num_of_elements_;
};
}  // namespace in_memory
}  // namespace redis_simple
