#include "memory/listpack.h"

#include <limits>
#include <memory>

#include "utils/string_utils.h"

namespace redis_simple {
namespace in_memory {
/*
 * Return the beginning index of the next element based on that of the current
 * element at the given index.
 */
size_t ListPack::Next(size_t idx) {
  size_t backlen = DecodeBacklen(idx);
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  return idx + backlen + backlen_bytes;
}

/*
 * Insert an element before/after/at the given position of the listpack.
 */
void ListPack::Insert(size_t idx, ListPack::Position where,
                      const std::string* elestr, int64_t* eleint) {
  if (!elestr && !eleint) return;
  if (where == Position::InsertAfter) {
    idx = Next(idx);
    where = Position::InsertBefore;
  }
  size_t backlen = 0;
  EncodingGeneralType encoding_type;
  if (eleint || (elestr && utils::ToInt64(*elestr, eleint))) {
    /* turn string to int64 value if applicable */
    backlen = EncodeInteger(nullptr, *eleint);
    encoding_type = EncodingGeneralType::typeInt;
  } else {
    backlen = EncodeString(nullptr, elestr);
    encoding_type = EncodingGeneralType::typeStr;
  }
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  size_t new_listpack_bytes = total_bytes_ + backlen + backlen_bytes;
  if (where == Position::InsertBefore) {
    /* insert a new  element before the existing element at the idx */
    Realloc(new_listpack_bytes);
    std::memmove(lp_ + idx + backlen + backlen_bytes, lp_ + idx,
                 total_bytes_ - idx);
    ++num_of_elements_;
  } else if (where == Position::Replace) {
    /* replace an existing element */
    size_t cur_backlen = DecodeBacklen(idx);
    size_t cur_backlen_bytes = GetBacklenBytes(cur_backlen);
    new_listpack_bytes -= (cur_backlen + cur_backlen_bytes);
    unsigned char* buf = Malloc(new_listpack_bytes);
    std::memcpy(buf, lp_, idx);
    std::memcpy(buf + idx + backlen + backlen_bytes,
                lp_ + idx + cur_backlen + cur_backlen_bytes,
                total_bytes_ - idx - cur_backlen - cur_backlen_bytes);
    Free();
    lp_ = buf;
  }
  total_bytes_ = new_listpack_bytes;
  if (encoding_type == EncodingGeneralType::typeInt) {
    EncodeInteger(lp_ + idx, *eleint);
  } else {
    EncodeString(lp_ + idx, elestr);
  }
}

void ListPack::Delete(size_t idx) {
  if (idx >= num_of_elements_) return;
  EncodingType encoding_type = GetEncodingType(idx);
  size_t backlen = DecodeBacklen(idx);
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  size_t new_listpack_bytes = total_bytes_ - backlen - backlen_bytes;
  unsigned char* buf = Malloc(new_listpack_bytes);
  std::memcpy(buf, lp_, idx);
  std::memcpy(buf + idx, lp_ + idx + backlen + backlen_bytes,
              total_bytes_ - idx - backlen - backlen_bytes);
  Free();
  lp_ = buf;
  total_bytes_ = new_listpack_bytes;
  --num_of_elements_;
}

/*
 * Encode the string into the given pointer of the listpack and return the back
 * length.
 */
size_t ListPack::EncodeString(unsigned char* const buf,
                              const std::string* ele) {
  const unsigned char* elestr =
      reinterpret_cast<const unsigned char*>(ele->c_str());
  size_t len = ele->size(), backlen = 0;
  if (len <= 63) {
    /* 6 bit length string */
    backlen = 1 + len;
    if (buf) {
      buf[0] = EncodingType::type6BitStr | len;
      memcpy(buf + 1, elestr, len);
    }
  } else if (len <= 4095) {
    /* 12 bit length string */
    backlen = 2 + len;
    if (buf) {
      buf[0] = EncodingType::type12BitStr | (len >> 8);
      buf[1] = len & 0xFF;
      memcpy(buf + 2, elestr, len);
    }
  } else {
    /* 32 bit length string */
    backlen = 5 + len;
    if (buf) {
      buf[0] = EncodingType::type32BitStr;
      buf[1] = len >> 24;
      buf[2] = (len >> 16) & 0XFF;
      buf[3] = (len >> 8) & 0XFF;
      buf[4] = len & 0xFF;
      memcpy(buf + 5, elestr, len);
    }
  }
  if (buf) {
    EncodeBacklen(buf + backlen, backlen);
  }
  return backlen;
}

/*
 * Encode the int64 into the given pointer of the listpack and return the back
 * length.
 */
size_t ListPack::EncodeInteger(unsigned char* const buf, int64_t ele) {
  size_t backlen = 0;
  if (ele >= 0 && ele <= 127) {
    /* 7 bit unsigned integer */
    backlen = 1;
    buf[0] = EncodingType::type7BitUInt | ele;
  } else if (ele >= -4096 && ele <= 4095) {
    /* 12 bit integer */
    backlen = 2;
    if (buf) {
      buf[0] = EncodingType::type13BitInt | (ele >> 8);
      buf[1] = ele & 0xFF;
    }
  } else if (ele >= std::numeric_limits<int16_t>::min() &&
             ele <= std::numeric_limits<int16_t>::max()) {
    /* 16 bit integer */
    backlen = 3;
    if (buf) {
      buf[0] = EncodingType::type16BitInt;
      buf[1] = (ele >> 8);
      buf[2] = ele & 0xFF;
    }
  } else if (ele >= int24BitIntMin && ele <= int24BitIntMax) {
    /* 24 bit integer */
    backlen = 4;
    if (buf) {
      buf[0] = EncodingType::type24BitInt;
      buf[1] = (ele >> 16);
      buf[2] = (ele >> 8) & 0xFF;
      buf[3] = ele & 0xFF;
    }
  } else if (ele >= std::numeric_limits<int32_t>::min() &&
             ele <= std::numeric_limits<int32_t>::max()) {
    /* 32 bit integer */
    backlen = 5;
    if (buf) {
      buf[0] = EncodingType::type32BitInt;
      buf[1] = (ele >> 24);
      buf[2] = (ele >> 16) & 0xFF;
      buf[3] = (ele >> 8) & 0xFF;
      buf[4] = ele & 0xFF;
    }
  } else {
    /* 64 bit integer */
    backlen = 9;
    if (buf) {
      buf[0] = EncodingType::type64BitInt;
      buf[1] = (ele >> 56);
      buf[2] = (ele >> 48) & 0xFF;
      buf[3] = (ele >> 40) & 0xFF;
      buf[4] = (ele >> 32) & 0xFF;
      buf[5] = (ele >> 24) & 0xFF;
      buf[6] = (ele >> 16) & 0xFF;
      buf[7] = (ele >> 8) & 0xFF;
      buf[8] = ele & 0xFF;
    }
  }
  if (buf) {
    EncodeBacklen(buf + backlen, backlen);
  }
  return backlen;
}

/*
 * Encode the back length to the given pointer of the listpack.
 */
void ListPack::EncodeBacklen(unsigned char* buf, size_t backlen) {
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  memcpy(buf, &backlen, backlen_bytes);
}

/*
 * Get the encoding type of the element at the given index of the listpack.
 */
ListPack::EncodingType ListPack::GetEncodingType(size_t idx) {
  const unsigned char* buf = lp_ + idx;
  if ((buf[0] & EncodingType::type7BitUInt) == EncodingType::type7BitUInt) {
    return EncodingType::type7BitUInt;
  }
  if ((buf[0] & EncodingType::type6BitStr) == EncodingType::type6BitStr) {
    return EncodingType::type6BitStr;
  }
  if ((buf[0] & EncodingType::type13BitInt) == EncodingType::type13BitInt) {
    return EncodingType::type13BitInt;
  }
  if ((buf[0] & EncodingType::type12BitStr) == EncodingType::type12BitStr) {
    return EncodingType::type12BitStr;
  }
  switch (buf[0]) {
    case EncodingType::type16BitInt:
      return EncodingType::type16BitInt;
    case EncodingType::type24BitInt:
      return EncodingType::type24BitInt;
    case EncodingType::type32BitInt:
      return EncodingType::type32BitInt;
    case EncodingType::type64BitInt:
      return EncodingType::type64BitInt;
    default:
      return EncodingType::type32BitStr;
  }
}

/*
 * Decode the string length from the given element of the listpack. The function
 * assumed that the element is of string type.
 */
size_t ListPack::DecodeStrLen(size_t idx) {
  const unsigned char* buf = lp_ + idx;
  switch (GetEncodingType(idx)) {
    case EncodingType::type6BitStr:
      return buf[0] & 0x3F;
    case EncodingType::type12BitStr:
      return ((buf[0] & 0xF) << 8) | buf[1];
    default:
      return (buf[1] << 24) | (buf[2] << 16) | (buf[2] << 8) | buf[3];
  }
}

/*
 * Get back length from the given element of the listpack. Note that the index
 * provided is the beginning of the element.
 */
size_t ListPack::DecodeBacklen(size_t idx) {
  switch (GetEncodingType(idx)) {
    case EncodingType::type7BitUInt:
      return 1;
    case EncodingType::type6BitStr:
      return 1 + DecodeStrLen(idx);
    case EncodingType::type13BitInt:
      return 2;
    case EncodingType::type12BitStr:
      return 2 + DecodeStrLen(idx);
    case EncodingType::type16BitInt:
      return 3;
    case EncodingType::type24BitInt:
      return 4;
    case EncodingType::type32BitInt:
      return 5;
    case EncodingType::type32BitStr:
      return 5 + DecodeStrLen(idx);
    default:
      return 9;
  }
}

/*
 * Get number of bytes required to encode the back length.
 */
uint8_t ListPack::GetBacklenBytes(size_t backlen) {
  if (backlen >= 0 && backlen <= BacklenThreshold::Size1ByteBacklenMax) {
    return 1;
  } else if (backlen > BacklenThreshold::Size1ByteBacklenMax &&
             backlen <= BacklenThreshold::Size2BytesBacklenMax) {
    return 2;
  } else if (backlen > BacklenThreshold::Size2BytesBacklenMax &&
             backlen <= BacklenThreshold::Size3BytesBacklenMax) {
    return 3;
  } else if (backlen > BacklenThreshold::Size3BytesBacklenMax &&
             backlen <= BacklenThreshold::Size4BytesBacklenMax) {
    return 4;
  } else {
    return 5;
  }
}

unsigned char* ListPack::Malloc(size_t bytes) {
  return new unsigned char[bytes];
}

void ListPack::Realloc(size_t bytes) {
  unsigned char* buf = new unsigned char[bytes];
  std::copy(lp_, lp_ + total_bytes_, buf);
  delete[] lp_;
  lp_ = buf;
}

void ListPack::Free() { delete[] lp_; }
}  // namespace in_memory
}  // namespace redis_simple
