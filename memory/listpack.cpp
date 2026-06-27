#include "memory/listpack.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

#include "utils/string_utils.h"

namespace redis_simple::in_memory {
namespace {
ssize_t ToSSize(size_t value) {
  if (value > static_cast<size_t>(std::numeric_limits<ssize_t>::max())) {
    return -1;
  }
  return static_cast<ssize_t>(value);
}
}  // namespace

ListPack::ListPack() : lp_(new unsigned char[kListPackHeaderSize + 1]) {
  SetTotalBytes(kListPackHeaderSize + 1);
  SetNumOfElements(0);
  lp_[kListPackHeaderSize] = kListPackEof;
}

ListPack::ListPack(size_t capacity)
    : lp_(new unsigned char[capacity > kListPackHeaderSize + 1
                                ? capacity
                                : kListPackHeaderSize + 1]) {
  capacity = std::max<size_t>(capacity, kListPackHeaderSize + 1);
  SetTotalBytes(capacity);
  SetNumOfElements(0);
  lp_[kListPackHeaderSize] = kListPackEof;
}

unsigned char* ListPack::Get(size_t idx, size_t* const len) const {
  size_t listpack_bytes = GetTotalBytes();
  if (idx < kListPackHeaderSize || idx >= listpack_bytes) {
    throw std::out_of_range("index out of bound");
  }
  if (idx == listpack_bytes - 1) {
    return nullptr;
  }
  EncodingType encoding_type = GetEncodingType(idx);
  if (IsString(encoding_type)) {
    return GetString(idx, len, encoding_type);
  }
  return GetInteger(idx, int_buf_, len, nullptr, encoding_type);
}

std::optional<std::string> ListPack::Get(size_t idx) const {
  size_t listpack_bytes = GetTotalBytes();
  if (idx < kListPackHeaderSize || idx >= listpack_bytes) {
    throw std::out_of_range("index out of bound");
  }
  if (idx == listpack_bytes - 1) {
    return std::nullopt;
  }

  size_t len = 0;
  unsigned char int_buf[kListPackIntBufSize];
  EncodingType encoding_type = GetEncodingType(idx);
  const unsigned char* buf =
      IsString(encoding_type)
          ? GetString(idx, &len, encoding_type)
          : GetInteger(idx, int_buf, &len, nullptr, encoding_type);
  return std::string(reinterpret_cast<const char*>(buf), len);
}

std::optional<int64_t> ListPack::GetInteger(size_t idx) const {
  size_t listpack_bytes = GetTotalBytes();
  if (idx < kListPackHeaderSize || idx >= listpack_bytes) {
    throw std::out_of_range("index out of bound");
  }
  if (idx == listpack_bytes - 1) {
    return std::nullopt;
  }
  EncodingType encoding_type = GetEncodingType(idx);
  if (IsString(encoding_type)) {
    return std::nullopt;
  }
  int64_t val = 0;
  GetInteger(idx, nullptr, nullptr, &val, encoding_type);
  return val;
}

ssize_t ListPack::Find(const std::string& val) const {
  return FindAndSkip(val, 0);
}

ssize_t ListPack::FindAndSkip(const std::string& val, size_t skip) const {
  ssize_t idx = First();
  if (idx < 0) {
    return -1;
  }
  size_t skip_count = 0;
  while (lp_[idx] != kListPackEof) {
    if (skip_count == 0) {
      const auto element = Get(idx);
      if (element.has_value() && *element == val) {
        return idx;
      }
      skip_count = skip;
    } else {
      --skip_count;
    }
    const size_t next_idx = Skip(static_cast<size_t>(idx));
    const ssize_t next = ToSSize(next_idx);
    if (next < 0) {
      return -1;
    }
    idx = next;
  }
  return -1;
}

bool ListPack::Append(const std::string& element_string) {
  uint32_t listpack_bytes = GetTotalBytes();
  return Insert(listpack_bytes - 1, Position::kInsertBefore, &element_string,
                nullptr);
}

bool ListPack::Append(int64_t element_integer) {
  uint32_t listpack_bytes = GetTotalBytes();
  return Insert(listpack_bytes - 1, Position::kInsertBefore, nullptr,
                &element_integer);
}

bool ListPack::Prepend(const std::string& element_string) {
  return Insert(kListPackHeaderSize, Position::kInsertBefore, &element_string,
                nullptr);
}

bool ListPack::Prepend(int64_t element_integer) {
  return Insert(kListPackHeaderSize, Position::kInsertBefore, nullptr,
                &element_integer);
}

bool ListPack::Insert(size_t idx, const std::string& element_string) {
  return Insert(idx, Position::kInsertBefore, &element_string, nullptr);
}

bool ListPack::Insert(size_t idx, int64_t element_integer) {
  return Insert(idx, Position::kInsertBefore, nullptr, &element_integer);
}

bool ListPack::Replace(size_t idx, const std::string& element_string) {
  return Insert(idx, Position::kReplace, &element_string, nullptr);
}

bool ListPack::Replace(size_t idx, int64_t element_integer) {
  return Insert(idx, Position::kReplace, nullptr, &element_integer);
}

bool ListPack::BatchAppend(const std::vector<ListPackEntry>& entries) {
  uint32_t listpack_bytes = GetTotalBytes();
  return BatchInsert(listpack_bytes - 1, Position::kInsertBefore, entries);
}

bool ListPack::BatchPrepend(const std::vector<ListPackEntry>& entries) {
  return BatchInsert(kListPackHeaderSize, Position::kInsertBefore, entries);
}

bool ListPack::BatchInsert(size_t idx,
                           const std::vector<ListPackEntry>& entries) {
  return BatchInsert(idx, Position::kInsertBefore, entries);
}

void ListPack::Delete(size_t idx) {
  uint32_t listpack_bytes = GetTotalBytes();
  if (idx < kListPackHeaderSize || idx >= listpack_bytes) {
    throw std::out_of_range("index out of bound");
  }
  if (idx == listpack_bytes - 1) {
    return;
  }
  size_t backlen = GetBacklen(idx);
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  size_t new_listpack_bytes = listpack_bytes - backlen - backlen_bytes;
  uint16_t num_of_elements = GetNumOfElements();
  std::memmove(lp_ + idx, lp_ + idx + backlen + backlen_bytes,
               listpack_bytes - idx - backlen - backlen_bytes);
  // Shrink after memmove so the source range remains valid.
  Realloc(new_listpack_bytes);
  SetTotalBytes(new_listpack_bytes);
  if (num_of_elements != kListPackNumEleUnknown) {
    SetNumOfElements(num_of_elements - 1);
  }
}

ssize_t ListPack::First() const {
  size_t listpack_bytes = GetTotalBytes();
  if (listpack_bytes <= ListPack::kListPackHeaderSize + 1) {
    return -1;
  }
  return ListPack::kListPackHeaderSize;
}

ssize_t ListPack::Last() const {
  size_t listpack_bytes = GetTotalBytes();
  if (listpack_bytes <= ListPack::kListPackHeaderSize + 1) {
    return -1;
  }
  return Prev(listpack_bytes - 1);
}

ssize_t ListPack::Next(size_t idx) const {
  if (idx < kListPackHeaderSize || idx >= GetTotalBytes()) {
    throw std::out_of_range("index out of bound");
  }
  if (lp_[idx] == kListPackEof) {
    return -1;
  }
  size_t next_idx = Skip(idx);
  return lp_[next_idx] != kListPackEof ? ToSSize(next_idx) : -1;
}

ssize_t ListPack::Prev(size_t idx) const {
  size_t listpack_bytes = GetTotalBytes();
  if (idx < kListPackHeaderSize || idx >= listpack_bytes) {
    throw std::out_of_range("index out of bound");
  }
  if (listpack_bytes <= kListPackHeaderSize + 1 || idx == kListPackHeaderSize) {
    return -1;
  }
  // Decode the backlen starting from the last byte of the previous element's
  // backlen.
  size_t backlen = DecodeBacklen(--idx);
  size_t backlen_bytes = GetBacklenBytes(backlen);
  return ToSSize(idx - backlen - backlen_bytes + 1);
}

unsigned char* ListPack::GetString(size_t idx, size_t* const len,
                                   EncodingType encoding_type) const {
  if (len != nullptr) {
    *len = DecodeStringLength(idx);
  }
  switch (encoding_type) {
    case EncodingType::k6BitString:
      return lp_ + idx + 1;
    case EncodingType::k12BitString:
      return lp_ + idx + 2;
    case EncodingType::k32BitString:
      return lp_ + idx + 5;
    default:
      return nullptr;
  }
}

unsigned char* ListPack::GetInteger(size_t idx, unsigned char* dst,
                                    size_t* const len, int64_t* sval,
                                    EncodingType encoding_type) const {
  int64_t val = 0;
  uint64_t uval = 0;
  uint64_t negstart = 0;
  uint64_t negmax = 0;
  switch (encoding_type) {
    case EncodingType::k7BitUnsignedInteger:
      uval = lp_[idx] & 0x7f;
      // Avoid sign conversion for the unsigned 7-bit format.
      negstart = std::numeric_limits<uint64_t>::max();
      break;
    case EncodingType::k13BitInteger:
      uval = ((lp_[idx] & 0x1f) << 8) | (lp_[idx + 1] & 0xff);
      negstart = 1 << 12;
      negmax = std::numeric_limits<uint16_t>::max() >> 3;
      break;
    case EncodingType::k16BitInteger:
      uval = (lp_[idx + 1] << 8) | (lp_[idx + 2]);
      negstart = 1 << 15;
      negmax = std::numeric_limits<uint16_t>::max();
      break;
    case EncodingType::k24BitInteger:
      uval = (lp_[idx + 1] << 16) | ((lp_[idx + 2]) << 8) | lp_[idx + 3];
      negstart = 1L << 23;
      negmax = std::numeric_limits<uint32_t>::max() >> 8;
      break;
    case EncodingType::k32BitInteger:
      uval = (static_cast<uint64_t>(lp_[idx + 1]) << 24) |
             (static_cast<uint64_t>(lp_[idx + 2]) << 16) |
             (static_cast<uint64_t>(lp_[idx + 3]) << 8) |
             static_cast<uint64_t>(lp_[idx + 4]);
      negstart = 1UL << 31;
      negmax = std::numeric_limits<uint32_t>::max();
      break;
    case EncodingType::k64BitInteger:
      uval = (static_cast<uint64_t>(lp_[idx + 1]) << 56) |
             (static_cast<uint64_t>(lp_[idx + 2]) << 48) |
             (static_cast<uint64_t>(lp_[idx + 3]) << 40) |
             (static_cast<uint64_t>(lp_[idx + 4]) << 32) |
             (static_cast<uint64_t>(lp_[idx + 5]) << 24) |
             (static_cast<uint64_t>(lp_[idx + 6]) << 16) |
             (static_cast<uint64_t>(lp_[idx + 7]) << 8) |
             static_cast<uint64_t>(lp_[idx + 8]);
      negstart = 1UL << 63;
      negmax = std::numeric_limits<uint64_t>::max();
      break;
    default:
      return nullptr;
  }
  if (uval >= negstart) {
    uval = negmax - uval;
    val = static_cast<int64_t>(uval);
    val = -val - 1;
  } else {
    val = static_cast<int64_t>(uval);
  }
  if (dst != nullptr) {
    *len = utils::LL2String(reinterpret_cast<char*>(dst), kListPackIntBufSize,
                            val);
    return dst;
  }
  *sval = val;

  return nullptr;
}

bool ListPack::Insert(size_t idx, ListPack::Position where,
                      const std::string* element_string,
                      const int64_t* element_integer) {
  if ((element_string == nullptr) && (element_integer == nullptr)) {
    return false;
  }
  uint32_t listpack_bytes = GetTotalBytes();
  if (idx < kListPackHeaderSize || idx >= listpack_bytes) {
    throw std::out_of_range("index out of bound");
  }
  if (where == Position::kInsertAfter) {
    idx = Skip(idx);
  }
  size_t backlen = 0;
  EncodingGeneralType encoding_type;
  int64_t sval = (element_integer != nullptr) ? *element_integer : 0;
  if ((element_integer != nullptr) || utils::ToInt64(*element_string, &sval)) {
    backlen = EncodeInteger(nullptr, sval);
    encoding_type = EncodingGeneralType::kInteger;
  } else {
    backlen = EncodeString(nullptr, element_string);
    encoding_type = EncodingGeneralType::kString;
  }
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  size_t replaced_bytes = 0;
  if (where == Position::kReplace) {
    size_t cur_backlen = GetBacklen(idx);
    size_t cur_backlen_bytes = GetBacklenBytes(cur_backlen);
    replaced_bytes = cur_backlen + cur_backlen_bytes;
  }
  size_t new_listpack_bytes =
      listpack_bytes + backlen + backlen_bytes - replaced_bytes;
  if (new_listpack_bytes > std::numeric_limits<uint32_t>::max()) {
    return false;
  }
  // Grow before memmove so the destination range is valid.
  if (new_listpack_bytes > listpack_bytes) {
    Realloc(new_listpack_bytes);
  }
  std::memmove(lp_ + idx + backlen + backlen_bytes, lp_ + idx + replaced_bytes,
               listpack_bytes - idx - replaced_bytes);
  if (where == Position::kInsertBefore) {
    uint16_t num_of_elements = GetNumOfElements();
    if (num_of_elements != kListPackNumEleUnknown) {
      SetNumOfElements(num_of_elements + 1);
    }
  }
  // Shrink after memmove so the source range remains valid.
  if (new_listpack_bytes < listpack_bytes) {
    Realloc(new_listpack_bytes);
  }
  SetTotalBytes(new_listpack_bytes);
  if (encoding_type == EncodingGeneralType::kInteger) {
    EncodeInteger(lp_ + idx, sval);
  } else {
    EncodeString(lp_ + idx, element_string);
  }
  return true;
}

bool ListPack::BatchInsert(size_t idx, ListPack::Position where,
                           const std::vector<ListPackEntry>& entries) {
  if (entries.empty()) {
    return false;
  }
  uint32_t listpack_bytes = GetTotalBytes();
  if (idx < kListPackHeaderSize || idx >= listpack_bytes) {
    throw std::out_of_range("index out of bound");
  }
  if (where == Position::kInsertAfter) {
    idx = Skip(idx);
  }
  std::vector<Encoding> encodings;
  size_t inserted_bytes = 0;
  // Precompute encodings before resizing so insertion can be one memmove.
  for (const ListPackEntry& entry : entries) {
    size_t backlen = 0;
    int64_t sval = entry.sval;
    if ((entry.str == nullptr) || utils::ToInt64(*(entry.str), &sval)) {
      backlen = EncodeInteger(nullptr, sval);
      Encoding encoding{};
      encoding.str = nullptr;
      encoding.sval = sval;
      encoding.encoding_type = EncodingGeneralType::kInteger;
      encoding.backlen_bytes = GetBacklenBytes(backlen);
      encodings.push_back(encoding);
    } else {
      backlen = EncodeString(nullptr, entry.str);
      Encoding encoding{};
      encoding.str = entry.str;
      encoding.sval = 0;
      encoding.encoding_type = EncodingGeneralType::kString;
      encoding.backlen_bytes = GetBacklenBytes(backlen);
      encodings.push_back(encoding);
    }
    inserted_bytes += (backlen + GetBacklenBytes(backlen));
  }
  size_t new_listpack_bytes = listpack_bytes + inserted_bytes;
  // Total bytes is a 4 byte unsigned integer, so the maximum bytes for the
  // listpack is UINT32_MAX.
  if (new_listpack_bytes > std::numeric_limits<uint32_t>::max()) {
    return false;
  }
  if (new_listpack_bytes < kListPackHeaderSize + 1) {
    return false;
  }
  uint16_t num_of_elements = GetNumOfElements();
  Realloc(new_listpack_bytes);
  std::memmove(lp_ + idx + inserted_bytes, lp_ + idx, listpack_bytes - idx);
  // Update number of elements and total bytes.
  if (num_of_elements != kListPackNumEleUnknown) {
    if (encodings.size() > kListPackNumEleUnknown - num_of_elements) {
      SetNumOfElements(kListPackNumEleUnknown);
    } else {
      SetNumOfElements(num_of_elements + entries.size());
    }
  }
  SetTotalBytes(new_listpack_bytes);
  // Insert elements based on encoding types.
  for (const Encoding& encoding : encodings) {
    if (encoding.encoding_type == EncodingGeneralType::kInteger) {
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

size_t ListPack::Size() const {
  uint16_t num_of_elements = GetNumOfElements();
  if (num_of_elements != kListPackNumEleUnknown) {
    return num_of_elements;
  }
  // Too many elements in the listpack, need to scan the entire listpack to get
  // the total number.
  size_t count = 0;
  ssize_t idx = First();
  while (idx != -1) {
    idx = Next(idx);
    ++count;
  }
  if (count < kListPackNumEleUnknown) {
    SetNumOfElements(count);
  }
  return count;
}

uint16_t ListPack::GetNumOfElements() const { return (lp_[4] << 8) | lp_[5]; }

void ListPack::SetNumOfElements(uint16_t num_of_elements) const {
  lp_[4] = (num_of_elements >> 8) & 0xff;
  lp_[5] = num_of_elements & 0xff;
}

/*
 * Return the beginning index of the next element of the current element at the
 * given index. If the index is pointing to the EOF, return the current index.
 */
size_t ListPack::Skip(size_t idx) const {
  if (lp_[idx] == kListPackEof) {
    return idx;
  }
  size_t backlen = GetBacklen(idx);
  uint8_t backlen_bytes = GetBacklenBytes(backlen);
  size_t next_idx = idx + backlen + backlen_bytes;
  return next_idx;
}

/*
 * Estimate the bytes needed to store the same integer multiple times.
 */
size_t ListPack::EstimateBytes(int64_t lval, size_t repeat) {
  return ListPack::kListPackHeaderSize +
         (EncodeInteger(nullptr, lval) * repeat) + 1;
}

/*
 * Return whether adding bytes keeps the listpack within the safe size limit.
 */
bool ListPack::SafeToAdd(const ListPack* const lp, size_t bytes) {
  size_t len = (lp != nullptr) ? lp->GetTotalBytes() : 0;
  return len + bytes <= ListPack::kListPackMaxSafetySize;
}

/*
 * Get the encoding type of the element at the given index of the listpack.
 * The function assumes that the index is valid.
 */
ListPack::EncodingType ListPack::GetEncodingType(size_t idx) const {
  const unsigned char* buf = lp_ + idx;
  if ((buf[0] & EncodingTypeMask::kType7BitUintMask) ==
      EncodingType::k7BitUnsignedInteger) {
    return EncodingType::k7BitUnsignedInteger;
  }
  if ((buf[0] & EncodingTypeMask::kType6BitStrMask) ==
      EncodingType::k6BitString) {
    return EncodingType::k6BitString;
  }
  if ((buf[0] & EncodingTypeMask::kType13BitIntMask) ==
      EncodingType::k13BitInteger) {
    return EncodingType::k13BitInteger;
  }
  if ((buf[0] & EncodingTypeMask::kType12BitStrMask) ==
      EncodingType::k12BitString) {
    return EncodingType::k12BitString;
  }
  switch (buf[0]) {
    case EncodingType::k16BitInteger:
      return EncodingType::k16BitInteger;
    case EncodingType::k24BitInteger:
      return EncodingType::k24BitInteger;
    case EncodingType::k32BitInteger:
      return EncodingType::k32BitInteger;
    case EncodingType::k64BitInteger:
      return EncodingType::k64BitInteger;
    default:
      return EncodingType::k32BitString;
  }
}

/*
 * Get back length from the given element of the listpack. Note that the index
 * provided is the beginning of the element.
 */
size_t ListPack::GetBacklen(size_t idx) const {
  switch (GetEncodingType(idx)) {
    case EncodingType::k7BitUnsignedInteger:
      return 1;
    case EncodingType::k6BitString:
      return 1 + DecodeStringLength(idx);
    case EncodingType::k13BitInteger:
      return 2;
    case EncodingType::k12BitString:
      return 2 + DecodeStringLength(idx);
    case EncodingType::k16BitInteger:
      return 3;
    case EncodingType::k24BitInteger:
      return 4;
    case EncodingType::k32BitInteger:
      return 5;
    case EncodingType::k32BitString:
      return 5 + DecodeStringLength(idx);
    default:
      return 9;
  }
}

/*
 * Get number of bytes required to encode the back length.
 */
uint8_t ListPack::GetBacklenBytes(size_t backlen) {
  if (backlen >= 0 && backlen <= BacklenThreshold::kSize1ByteBacklenMax) {
    return 1;
  }
  if (backlen > BacklenThreshold::kSize1ByteBacklenMax &&
      backlen <= BacklenThreshold::kSize2BytesBacklenMax) {
    return 2;
  } else if (backlen > BacklenThreshold::kSize2BytesBacklenMax &&
             backlen <= BacklenThreshold::kSize3BytesBacklenMax) {
    return 3;
  } else if (backlen > BacklenThreshold::kSize3BytesBacklenMax &&
             backlen <= BacklenThreshold::kSize4BytesBacklenMax) {
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
  const auto* element_string =
      reinterpret_cast<const unsigned char*>(ele->c_str());
  size_t len = ele->size();
  size_t backlen = 0;
  if (len <= 63) {
    // 6 bit length string
    backlen = 1 + len;
    if (buf != nullptr) {
      buf[0] = EncodingType::k6BitString | len;
      std::memcpy(buf + 1, element_string, len);
    }
  } else if (len <= 4095) {
    // 12 bit length string
    backlen = 2 + len;
    if (buf != nullptr) {
      buf[0] = EncodingType::k12BitString | (len >> 8);
      buf[1] = len & 0xff;
      std::memcpy(buf + 2, element_string, len);
    }
  } else {
    // 32 bit length string
    backlen = 5 + len;
    if (buf != nullptr) {
      buf[0] = EncodingType::k32BitString;
      buf[1] = len >> 24;
      buf[2] = (len >> 16) & 0xff;
      buf[3] = (len >> 8) & 0xff;
      buf[4] = len & 0xff;
      std::memcpy(buf + 5, element_string, len);
    }
  }
  if (buf != nullptr) {
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
  if (v >= 0 && v <= kUint7BitIntMax) {
    // 7 bit unsigned integer
    backlen = 1;
    if (buf != nullptr) {
      buf[0] = EncodingType::k7BitUnsignedInteger | v;
    }
  } else if (v >= -4096 && v <= 4095) {
    // 13 bit integer
    if (v < 0) {
      v += (1 << 13);
    }
    backlen = 2;
    if (buf != nullptr) {
      buf[0] = EncodingType::k13BitInteger | (v >> 8);
      buf[1] = v & 0xff;
    }
  } else if (v >= std::numeric_limits<int16_t>::min() &&
             v <= std::numeric_limits<int16_t>::max()) {
    // 16 bit integer
    if (v < 0) {
      v += (1 << 16);
    }
    backlen = 3;
    if (buf != nullptr) {
      buf[0] = EncodingType::k16BitInteger;
      buf[1] = (v >> 8);
      buf[2] = v & 0xff;
    }
  } else if (v >= kInt24BitIntMin && v <= kInt24BitIntMax) {
    // 24 bit integer
    if (v < 0) {
      v += (1 << 24);
    }
    backlen = 4;
    if (buf != nullptr) {
      buf[0] = EncodingType::k24BitInteger;
      buf[1] = (v >> 16);
      buf[2] = (v >> 8) & 0xff;
      buf[3] = v & 0xff;
    }
  } else if (v >= std::numeric_limits<int32_t>::min() &&
             v <= std::numeric_limits<int32_t>::max()) {
    // 32 bit integer
    if (v < 0) {
      v += (static_cast<int64_t>(1) << 32);
    }
    backlen = 5;
    if (buf != nullptr) {
      buf[0] = EncodingType::k32BitInteger;
      buf[1] = (v >> 24);
      buf[2] = (v >> 16) & 0xff;
      buf[3] = (v >> 8) & 0xff;
      buf[4] = v & 0xff;
    }
  } else {
    // 64 bit integer
    uint64_t uv = v;
    backlen = 9;
    if (buf != nullptr) {
      buf[0] = EncodingType::k64BitInteger;
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
  if (buf != nullptr) {
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
      // Set the most significant bit to 1 if this is not the first byte of the
      // backlen.
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
  } while ((lp_[idx--] & 128) != 0);
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
    case EncodingType::k6BitString:
      return buf[0] & 0x3f;
    case EncodingType::k12BitString:
      return ((buf[0] & 0xf) << 8) | buf[1];
    default:
      return (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
  }
}

bool ListPack::IsString(EncodingType encoding_type) {
  return encoding_type == EncodingType::k6BitString ||
         encoding_type == EncodingType::k12BitString ||
         encoding_type == EncodingType::k32BitString;
}

void ListPack::Realloc(size_t bytes) {
  if (bytes < kListPackHeaderSize + 1) {
    throw std::length_error("listpack allocation is smaller than its header");
  }
  auto* new_lp = new unsigned char[bytes];
  const size_t copy_bytes =
      std::min(static_cast<size_t>(GetTotalBytes()), bytes);
  std::memcpy(new_lp, lp_, copy_bytes);
  delete[] lp_;
  lp_ = new_lp;
}

void ListPack::Free() { delete[] lp_; }
}  // namespace redis_simple::in_memory
