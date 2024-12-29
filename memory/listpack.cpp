#include "memory/listpack.h"

#include <limits>
#include <memory>

#include "utils/string_utils.h"

namespace redis_simple {
namespace in_memory {
ListPack::ListPack() : lp_(new unsigned char[ListPackHeaderSize + 1]) {
  SetTotalBytes(ListPackHeaderSize + 1);
  SetNumOfElements(0);
  lp_[ListPackHeaderSize] = ListPackEOF;
}

ListPack::ListPack(size_t capacity)
    : lp_(new unsigned char[capacity > ListPackHeaderSize + 1
                                ? capacity
                                : ListPackHeaderSize + 1]) {
  if (capacity < ListPackHeaderSize + 1) capacity = ListPackHeaderSize + 1;
  SetTotalBytes(capacity);
  SetNumOfElements(0);
  lp_[ListPackHeaderSize] = ListPackEOF;
}

/*
 * Get an element with buffer from the listpack.  The function assumes the index
 * is the beginning of an element if valid. Store the length of the buffer into
 * the variable `len`.
 */
unsigned char* ListPack::Get(size_t idx, size_t* const len) const {
  size_t listpack_bytes = GetTotalBytes();
  /* index out of bound, < header size or >= total bytes */
  if (idx < ListPackHeaderSize || idx >= listpack_bytes)
    throw std::out_of_range("index out of bound");
  /* The end of the list */
  if (idx == listpack_bytes - 1) return nullptr;
  EncodingType encoding_type = GetEncodingType(idx);
  if (isString(encoding_type)) {
    /* Get string */
    return GetString(idx, len, encoding_type);
  } else {
    /* Get integer */
    unsigned char* dst = new unsigned char[ListPackIntBufSize];
    return GetInteger(idx, dst, len, nullptr, encoding_type);
  }
}

std::optional<std::string> ListPack::Get(size_t idx) const {
  size_t len = 0;
  const unsigned char* buf = Get(idx, &len);
  if (!buf) return std::nullopt;
  return std::string(reinterpret_cast<const char*>(buf), len);
}

/*
 * Get an integer from the listpack. The function assumes the index is the
 * beginning of an element if valid.
 */
std::optional<int64_t> ListPack::GetInteger(size_t idx) const {
  size_t listpack_bytes = GetTotalBytes();
  /* index out of bound, < header size or >= total bytes */
  if (idx < ListPackHeaderSize || idx >= listpack_bytes)
    throw std::out_of_range("index out of bound");
  /* The end of the list */
  if (idx == listpack_bytes - 1) return std::nullopt;
  EncodingType encoding_type = GetEncodingType(idx);
  if (isString(encoding_type)) return std::nullopt;
  int64_t val;
  GetInteger(idx, nullptr, nullptr, &val, encoding_type);
  return val;
}

/*
 * Find the index of an element in the listpack. If the element does not exist,
 * return -1.
 */
ssize_t ListPack::Find(const std::string& val) const {
  return FindAndSkip(val, 0);
}

/*
 * Find the index of an element in the listpack skipping `skip` entries between
 * each comparison. If the element does not exist, return -1.
 */
ssize_t ListPack::FindAndSkip(const std::string& val, size_t skip) const {
  ssize_t idx = First();
  if (idx < 0) return -1;
  size_t skipcnt = 0;
  while (lp_[idx] != ListPackEOF) {
    if (skipcnt == 0) {
      size_t len = 0;
      const unsigned char* buf = Get(idx, &len);
      if (len == val.size() && std::equal(buf, buf + len, val.c_str())) {
        return idx;
      }
      skipcnt = skip;
    } else {
      --skipcnt;
    }
    idx = Skip(idx);
  }
  return -1;
}

/*
 * Append a string to the end of the listpack.
 */
bool ListPack::Append(const std::string& elestr) {
  uint32_t listpack_bytes = GetTotalBytes();
  return Insert(listpack_bytes - 1, Position::InsertBefore, &elestr, nullptr);
}

/*
 * Append an integer to the end of the listpack.
 */
bool ListPack::Append(int64_t eleint) {
  uint32_t listpack_bytes = GetTotalBytes();
  return Insert(listpack_bytes - 1, Position::InsertBefore, nullptr, &eleint);
}

/*
 * Prepend a string at the beginning of the listpack.
 */
bool ListPack::Prepend(const std::string& elestr) {
  return Insert(ListPackHeaderSize, Position::InsertBefore, &elestr, nullptr);
}

/*
 * Prepend an integer at the beginning of the listpack.
 */
bool ListPack::Prepend(int64_t eleint) {
  return Insert(ListPackHeaderSize, Position::InsertBefore, nullptr, &eleint);
}

/*
 * Insert a string at the given index of the listpack.
 */
bool ListPack::Insert(size_t idx, const std::string& elestr) {
  return Insert(idx, Position::InsertBefore, &elestr, nullptr);
}

/*
 * Insert an integer at the given index of the listpack.
 */
bool ListPack::Insert(size_t idx, int64_t eleint) {
  return Insert(idx, Position::InsertBefore, nullptr, &eleint);
}

/*
 * Replace a string at the given index of the listpack.
 */
bool ListPack::Replace(size_t idx, const std::string& elestr) {
  return Insert(idx, Position::Replace, &elestr, nullptr);
}

/*
 * Replace an integer at the given index of the listpack.
 */
bool ListPack::Replace(size_t idx, int64_t eleint) {
  return Insert(idx, Position::Replace, nullptr, &eleint);
}

/*
 * Batch append elements to the end of the listpack.
 */
bool ListPack::BatchAppend(const std::vector<ListPackEntry>& entries) {
  uint32_t listpack_bytes = GetTotalBytes();
  return BatchInsert(listpack_bytes - 1, Position::InsertBefore, entries);
}

/*
 * Batch prepend elements to the beginning of the listpack.
 */
bool ListPack::BatchPrepend(const std::vector<ListPackEntry>& entries) {
  return BatchInsert(ListPackHeaderSize, Position::InsertBefore, entries);
}

/*
 * Batch insert elements at the given index.
 */
bool ListPack::BatchInsert(size_t idx,
                           const std::vector<ListPackEntry>& entries) {
  return BatchInsert(idx, Position::InsertBefore, entries);
}

/*
 * Delete the element at the given index.
 */
void ListPack::Delete(size_t idx) {
  uint32_t listpack_bytes = GetTotalBytes();
  /* cannot insert into the listpack header or out of the listpack bound */
  if (idx < ListPackHeaderSize || idx >= listpack_bytes)
    throw std::out_of_range("index out of bound");
  /* The end of the list */
  if (idx == listpack_bytes - 1) return;
  EncodingType encoding_type = GetEncodingType(idx);
  size_t backlen = GetBacklen(idx);
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  size_t new_listpack_bytes = listpack_bytes - backlen - backlen_bytes;
  unsigned char* buf = Malloc(new_listpack_bytes);
  std::memcpy(buf, lp_, idx);
  std::memcpy(buf + idx, lp_ + idx + backlen + backlen_bytes,
              listpack_bytes - idx - backlen - backlen_bytes);
  Free();
  lp_ = buf;
  SetTotalBytes(new_listpack_bytes);
  uint16_t num_of_elements = GetNumOfElements();
  SetNumOfElements(num_of_elements - 1);
}

/*
 * Return the beginning index (which should be ListPackHeaderSize) of the first
 * element in the listpack. If the listpack is empty, return -1.
 */
ssize_t ListPack::First() const {
  size_t listpack_bytes = GetTotalBytes();
  if (listpack_bytes <= ListPack::ListPackHeaderSize + 1) return -1;
  return ListPack::ListPackHeaderSize;
}

/*
 * Return the beginning index (which should be ListPackHeaderSize) of the last
 * element in the listpack. If the listpack is empty, return -1.
 */
ssize_t ListPack::Last() const {
  size_t listpack_bytes = GetTotalBytes();
  /* no element in the listpack */
  if (listpack_bytes <= ListPack::ListPackHeaderSize + 1) return -1;
  return Prev(listpack_bytes - 1);
}

/*
 * Return the beginning index of the next element of the element at the
 * given index. If there is no more element, return -1.
 */
ssize_t ListPack::Next(size_t idx) const {
  /* Index out of bound, < header size or >= total bytes */
  if (idx < ListPackHeaderSize || idx >= GetTotalBytes())
    throw std::out_of_range("index out of bound");
  if (lp_[idx] == ListPackEOF) return -1;
  size_t next_idx = Skip(idx);
  /* return -1 if the element is the last one */
  return lp_[next_idx] != ListPackEOF ? next_idx : -1;
}

/*
 * Return the beginning index of the previous element of the element at
 * the given index. If there is no more element, return -1.
 */
ssize_t ListPack::Prev(size_t idx) const {
  size_t listpack_bytes = GetTotalBytes();
  /* Index out of bound, < header size or >= total bytes */
  if (idx < ListPackHeaderSize || idx >= listpack_bytes)
    throw std::out_of_range("index out of bound");
  /* no element in the listpack or the element is the first one */
  if (listpack_bytes <= ListPackHeaderSize + 1 || idx == ListPackHeaderSize)
    return -1;
  /* Decode the backlen starting from the last byte of the previous element's
   * backlen */
  size_t backlen = DecodeBacklen(--idx);
  size_t backlen_bytes = GetBacklenBytes(backlen);
  return idx - backlen - backlen_bytes + 1;
}

/*
 * Get string from the given index based on the encoding type.
 * The function assumes that the element at the index is a valid string.
 */
unsigned char* ListPack::GetString(size_t idx, size_t* const len,
                                   EncodingType encoding_type) const {
  if (len) *len = DecodeStringLength(idx);
  switch (encoding_type) {
    case EncodingType::type6BitStr:
      return lp_ + idx + 1;
    case EncodingType::type12BitStr:
      return lp_ + idx + 2;
    case EncodingType::type32BitStr:
      return lp_ + idx + 5;
    default:
      return nullptr;
  }
}

/*
 * Get string from the given index based on the encoding type.
 * The function assumes that the element at the index is a valid 64 bit integer.
 */
unsigned char* ListPack::GetInteger(size_t idx, unsigned char* dst,
                                    size_t* const len, int64_t* sval,
                                    EncodingType encoding_type) const {
  int64_t val = 0;
  uint64_t uval = 0, negstart = 0, negmax = 0;
  switch (encoding_type) {
    case EncodingType::type7BitUInt:
      uval = lp_[idx] & 0x7f;
      /* no need to convert unsigned to signed number in this case. Set negstart
       * to a number absolutely larger than the uval */
      negstart = std::numeric_limits<uint64_t>::max();
      break;
    case EncodingType::type13BitInt:
      uval = ((lp_[idx] & 0x1f) << 8) | (lp_[idx + 1] & 0xff);
      negstart = 1 << 12;
      negmax = std::numeric_limits<uint16_t>::max() >> 3;
      break;
    case EncodingType::type16BitInt:
      uval = (lp_[idx + 1] << 8) | (lp_[idx + 2]);
      negstart = 1 << 15;
      negmax = std::numeric_limits<uint16_t>::max();
      break;
    case EncodingType::type24BitInt:
      uval = (lp_[idx + 1] << 16) | ((lp_[idx + 2]) << 8) | lp_[idx + 3];
      negstart = 1L << 23;
      negmax = std::numeric_limits<uint32_t>::max() >> 8;
      break;
    case EncodingType::type32BitInt:
      uval = ((uint64_t)lp_[idx + 1] << 24) | ((uint64_t)lp_[idx + 2] << 16) |
             ((uint64_t)lp_[idx + 3] << 8) | (uint64_t)lp_[idx + 4];
      negstart = 1UL << 31;
      negmax = std::numeric_limits<uint32_t>::max();
      break;
    case EncodingType::type64BitInt:
      uval = ((uint64_t)lp_[idx + 1] << 56) | ((uint64_t)lp_[idx + 2] << 48) |
             ((uint64_t)lp_[idx + 3] << 40) | ((uint64_t)lp_[idx + 4] << 32) |
             ((uint64_t)lp_[idx + 5] << 24) | ((uint64_t)lp_[idx + 6] << 16) |
             ((uint64_t)lp_[idx + 7] << 8) | (uint64_t)lp_[idx + 8];
      negstart = 1UL << 63;
      negmax = std::numeric_limits<uint64_t>::max();
      break;
    default:
      return nullptr;
  }
  if (uval >= negstart) {
    uval = negmax - uval;
    val = uval;
    val = -val - 1;
  } else {
    val = uval;
  }
  if (dst) {
    *len =
        utils::LL2String(reinterpret_cast<char*>(dst), ListPackIntBufSize, val);
    return dst;
  } else {
    /* if dst is null, return the integer instead of the int buffer */
    *sval = val;
  }
  return nullptr;
}

/*
 * Insert an element before/after the given position of the listpack or replace
 * the element at the given position of the listpack.
 */
bool ListPack::Insert(size_t idx, ListPack::Position where,
                      const std::string* elestr, int64_t* eleint) {
  if (!elestr && !eleint) return false;
  uint32_t listpack_bytes = GetTotalBytes();
  /* cannot insert into the listpack header or out of the listpack bound */
  if (idx < ListPackHeaderSize || idx >= listpack_bytes)
    throw std::out_of_range("index out of bound");
  if (where == Position::InsertAfter) {
    idx = Skip(idx);
    where = Position::InsertBefore;
  }
  size_t backlen = 0;
  EncodingGeneralType encoding_type;
  int64_t sval = eleint ? *eleint : 0;
  if (eleint || utils::ToInt64(*elestr, &sval)) {
    /* turn string to int64 value if applicable */
    backlen = EncodeInteger(nullptr, sval);
    encoding_type = EncodingGeneralType::typeInt;
  } else {
    backlen = EncodeString(nullptr, elestr);
    encoding_type = EncodingGeneralType::typeStr;
  }
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  size_t replaced_bytes = 0;
  if (where == Position::Replace) {
    /* replace an existing element */
    size_t cur_backlen = GetBacklen(idx);
    size_t cur_backlen_bytes = GetBacklenBytes(cur_backlen);
    replaced_bytes = cur_backlen + cur_backlen_bytes;
  }
  size_t new_listpack_bytes =
      listpack_bytes + backlen + backlen_bytes - replaced_bytes;
  /* total bytes is a 4 byte unsigned integer, so the maximum bytes for the
   * listpack is UINT32_MAX */
  if (new_listpack_bytes > std::numeric_limits<uint32_t>::max()) return false;
  if (where == Position::InsertBefore) {
    /* insert a new  element before the existing element at the idx */
    Realloc(new_listpack_bytes);
    std::memmove(lp_ + idx + backlen + backlen_bytes, lp_ + idx,
                 listpack_bytes - idx);
    uint16_t num_of_elements = GetNumOfElements();
    SetNumOfElements(num_of_elements + 1);
  } else if (where == Position::Replace) {
    /* replace an existing element */
    unsigned char* buf = Malloc(new_listpack_bytes);
    std::memcpy(buf, lp_, idx);
    std::memcpy(buf + idx + backlen + backlen_bytes, lp_ + idx + replaced_bytes,
                listpack_bytes - idx - replaced_bytes);
    /* release the old listpack */
    Free();
    lp_ = buf;
  }
  SetTotalBytes(new_listpack_bytes);
  if (encoding_type == EncodingGeneralType::typeInt) {
    EncodeInteger(lp_ + idx, sval);
  } else {
    EncodeString(lp_ + idx, elestr);
  }
  return true;
}

/*
 * Batch insert elements before/after the given position of the listpack.
 */
bool ListPack::BatchInsert(size_t idx, ListPack::Position where,
                           const std::vector<ListPackEntry>& entries) {
  if (entries.empty()) return false;
  uint32_t listpack_bytes = GetTotalBytes();
  /* cannot insert into the listpack header or out of the listpack bound */
  if (idx < ListPackHeaderSize || idx >= listpack_bytes)
    throw std::out_of_range("index out of bound");
  if (where == Position::InsertAfter) {
    idx = Skip(idx);
    where = Position::InsertBefore;
  }
  std::vector<Encoding> encodings;
  size_t inserted_bytes = 0;
  /* Get general encoding type (string/int) and backlen from each entry */
  for (const ListPackEntry& entry : entries) {
    size_t backlen = 0;
    int64_t sval = entry.sval;
    if (!entry.str || utils::ToInt64(*(entry.str), &sval)) {
      backlen = EncodeInteger(nullptr, sval);
      encodings.push_back({
          .sval = sval,
          .encoding_type = EncodingGeneralType::typeInt,
          .backlen_bytes = GetBacklenBytes(backlen),
      });
    } else {
      backlen = EncodeString(nullptr, entry.str);
      encodings.push_back({
          .str = entry.str,
          .encoding_type = EncodingGeneralType::typeStr,
          .backlen_bytes = GetBacklenBytes(backlen),
      });
    }
    inserted_bytes += (backlen + GetBacklenBytes(backlen));
  }
  size_t new_listpack_bytes = listpack_bytes + inserted_bytes;
  /* total bytes is a 4 byte unsigned integer, so the maximum bytes for the
   * listpack is UINT32_MAX */
  if (new_listpack_bytes > std::numeric_limits<uint32_t>::max()) return false;
  Realloc(new_listpack_bytes);
  std::memmove(lp_ + idx + inserted_bytes, lp_ + idx, listpack_bytes - idx);
  /* update number of elements and total bytes */
  uint16_t num_of_elements = GetNumOfElements();
  SetNumOfElements(num_of_elements + entries.size());
  SetTotalBytes(new_listpack_bytes);
  /* Insert elements based on encoding types. */
  for (const Encoding& encoding : encodings) {
    if (encoding.encoding_type == EncodingGeneralType::typeInt) {
      idx += EncodeInteger(lp_ + idx, encoding.sval);
    } else {
      idx += EncodeString(lp_ + idx, encoding.str);
    }
    idx += encoding.backlen_bytes;
  }
  return true;
}

uint32_t ListPack::GetTotalBytes() const {
  return (lp_[0] << 24) | (lp_[1] << 16) | (lp_[2] << 8) | lp_[3];
}

void ListPack::SetTotalBytes(uint32_t listpack_bytes) {
  lp_[0] = (listpack_bytes >> 24) & 0xff;
  lp_[1] = (listpack_bytes >> 16) & 0xff;
  lp_[2] = (listpack_bytes >> 8) & 0xff;
  lp_[3] = listpack_bytes & 0xff;
}

uint16_t ListPack::GetNumOfElements() const { return (lp_[4] << 8) | lp_[5]; }

void ListPack::SetNumOfElements(uint16_t num_of_elements) {
  lp_[4] = (num_of_elements >> 8) & 0xff;
  lp_[5] = num_of_elements & 0xff;
}

/*
 * Return the beginning index of the next element of the current element at the
 * given index. If the index is pointing to the EOF, return the current index.
 */
size_t ListPack::Skip(size_t idx) const {
  if (lp_[idx] == ListPackEOF) return idx;
  size_t backlen = GetBacklen(idx);
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  size_t next_idx = idx + backlen + backlen_bytes;
  return next_idx;
}

/*
 * Estimate number of bytes needed to store an integer for repetive times.
 */
size_t ListPack::EstimateBytes(int64_t lval, size_t repetive) {
  return ListPack::ListPackHeaderSize +
         EncodeInteger(nullptr, lval) * repetive + 1;
}

/*
 * Estimate number of bytes needed to store an integer for repetive times.
 */
bool ListPack::SafeToAdd(const ListPack* const lp, size_t bytes) {
  size_t len = lp ? lp->GetTotalBytes() : 0;
  return len + bytes < ListPack::ListPackMaxSafetySize;
}

/*
 * Get the encoding type of the element at the given index of the listpack.
 * The function assumes that the index is valid.
 */
ListPack::EncodingType ListPack::GetEncodingType(size_t idx) const {
  const unsigned char* buf = lp_ + idx;
  if ((buf[0] & EncodingTypeMask::type7BitUIntMask) ==
      EncodingType::type7BitUInt) {
    return EncodingType::type7BitUInt;
  }
  if ((buf[0] & EncodingTypeMask::type6BitStrMask) ==
      EncodingType::type6BitStr) {
    return EncodingType::type6BitStr;
  }
  if ((buf[0] & EncodingTypeMask::type13BitIntMask) ==
      EncodingType::type13BitInt) {
    return EncodingType::type13BitInt;
  }
  if ((buf[0] & EncodingTypeMask::type12BitStrMask) ==
      EncodingType::type12BitStr) {
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
 * Get back length from the given element of the listpack. Note that the index
 * provided is the beginning of the element.
 */
size_t ListPack::GetBacklen(size_t idx) const {
  switch (GetEncodingType(idx)) {
    case EncodingType::type7BitUInt:
      return 1;
    case EncodingType::type6BitStr:
      return 1 + DecodeStringLength(idx);
    case EncodingType::type13BitInt:
      return 2;
    case EncodingType::type12BitStr:
      return 2 + DecodeStringLength(idx);
    case EncodingType::type16BitInt:
      return 3;
    case EncodingType::type24BitInt:
      return 4;
    case EncodingType::type32BitInt:
      return 5;
    case EncodingType::type32BitStr:
      return 5 + DecodeStringLength(idx);
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

/*
 * Encode the string into the given pointer of the listpack and return the
 * number of bytes needed to encode the element (not include bytes used to
 * encode back length).
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
      std::memcpy(buf + 1, elestr, len);
    }
  } else if (len <= 4095) {
    /* 12 bit length string */
    backlen = 2 + len;
    if (buf) {
      buf[0] = EncodingType::type12BitStr | (len >> 8);
      buf[1] = len & 0xff;
      std::memcpy(buf + 2, elestr, len);
    }
  } else {
    /* 32 bit length string */
    backlen = 5 + len;
    if (buf) {
      buf[0] = EncodingType::type32BitStr;
      buf[1] = len >> 24;
      buf[2] = (len >> 16) & 0xff;
      buf[3] = (len >> 8) & 0xff;
      buf[4] = len & 0xff;
      std::memcpy(buf + 5, elestr, len);
    }
  }
  if (buf) {
    EncodeBacklen(buf + backlen, backlen);
  }
  return backlen;
}

/*
 * Encode the int64 into the given pointer of the listpack and return the number
 * of bytes needed to encode the element (not include bytes used to encode back
 * length).
 */
size_t ListPack::EncodeInteger(unsigned char* const buf, int64_t v) {
  size_t backlen = 0;
  if (v >= 0 && v <= 127) {
    /* 7 bit unsigned integer */
    backlen = 1;
    if (buf) {
      buf[0] = EncodingType::type7BitUInt | v;
    }
  } else if (v >= -4096 && v <= 4095) {
    /* 13 bit integer */
    if (v < 0) v += (1 << 13);
    backlen = 2;
    if (buf) {
      buf[0] = EncodingType::type13BitInt | (v >> 8);
      buf[1] = v & 0xff;
    }
  } else if (v >= std::numeric_limits<int16_t>::min() &&
             v <= std::numeric_limits<int16_t>::max()) {
    /* 16 bit integer */
    if (v < 0) v += (1 << 16);
    backlen = 3;
    if (buf) {
      buf[0] = EncodingType::type16BitInt;
      buf[1] = (v >> 8);
      buf[2] = v & 0xff;
    }
  } else if (v >= Int24BitIntMin && v <= Int24BitIntMax) {
    /* 24 bit integer */
    if (v < 0) v += (1 << 24);
    backlen = 4;
    if (buf) {
      buf[0] = EncodingType::type24BitInt;
      buf[1] = (v >> 16);
      buf[2] = (v >> 8) & 0xff;
      buf[3] = v & 0xff;
    }
  } else if (v >= std::numeric_limits<int32_t>::min() &&
             v <= std::numeric_limits<int32_t>::max()) {
    /* 32 bit integer */
    if (v < 0) v += ((int64_t)1 << 32);
    backlen = 5;
    if (buf) {
      buf[0] = EncodingType::type32BitInt;
      buf[1] = (v >> 24);
      buf[2] = (v >> 16) & 0xff;
      buf[3] = (v >> 8) & 0xff;
      buf[4] = v & 0xff;
    }
  } else {
    /* 64 bit integer */
    uint64_t uv = v;
    backlen = 9;
    if (buf) {
      buf[0] = EncodingType::type64BitInt;
      buf[1] = (uv >> 56);
      buf[2] = (uv >> 48) & 0xff;
      buf[3] = (uv >> 40) & 0xff;
      buf[4] = (uv >> 32) & 0xff;
      buf[5] = (uv >> 24) & 0xff;
      buf[6] = (uv >> 16) & 0xff;
      buf[7] = (uv >> 8) & 0xff;
      buf[8] = uv & 0xff;
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
void ListPack::EncodeBacklen(unsigned char* const buf, size_t backlen) {
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  for (uint8_t i = 0; i < backlen_bytes; ++i) {
    buf[i] = backlen & 127;
    if (i > 0) {
      /* Set the most significant bit to 1 if this is not the first byte of the
       * backlen */
      buf[i] |= 128;
    }
    backlen >>= 7;
  }
}

/*
 * Decode the backlen of the element given its last byte through traversing
 * reversely to find the first byte has its most siginificant bit set to zero.
 * The function assumes that the index is valid and is the last byte of the
 * element.
 */
size_t ListPack::DecodeBacklen(size_t idx) const {
  size_t backlen = 0;
  do {
    backlen = (backlen << 7) | (lp_[idx] & 127);
  } while (lp_[idx--] & 128);
  return backlen;
}

/*
 * Decode the string length from the given element of the listpack.
 * The function assumes that the index is valid and the element at the index is
 * of string type.
 */
size_t ListPack::DecodeStringLength(size_t idx) const {
  const unsigned char* buf = lp_ + idx;
  switch (GetEncodingType(idx)) {
    case EncodingType::type6BitStr:
      return buf[0] & 0x3f;
    case EncodingType::type12BitStr:
      return ((buf[0] & 0xf) << 8) | buf[1];
    default:
      return (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
  }
}

bool ListPack::isString(EncodingType encoding_type) {
  return encoding_type == EncodingType::type6BitStr ||
         encoding_type == EncodingType::type12BitStr ||
         encoding_type == EncodingType::type32BitStr;
}

unsigned char* ListPack::Malloc(size_t bytes) {
  return new unsigned char[bytes];
}

void ListPack::Realloc(size_t bytes) {
  unsigned char* buf = new unsigned char[bytes];
  uint32_t listpack_bytes = GetTotalBytes();
  std::memcpy(buf, lp_, listpack_bytes);
  Free();
  lp_ = buf;
}

void ListPack::Free() { delete[] lp_; }
}  // namespace in_memory
}  // namespace redis_simple
