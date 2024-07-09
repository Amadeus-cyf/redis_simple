#pragma once

#include <string>
#include <variant>

#include "server/zset/zset.h"

namespace redis_simple {
namespace db {
class RedisObj {
 private:
  using DataType = std::variant<std::string, zset::ZSet*>;

 public:
  enum class ObjEncoding {
    objEncodingString = 1,
    objEncodingZSet = 2,
  };

  static RedisObj* CreateWithString(const std::string& val) {
    return Create(ObjEncoding::objEncodingString, val);
  }
  static RedisObj* CreateWithZSet(zset::ZSet* const zset) {
    return Create(ObjEncoding::objEncodingZSet, zset);
  }
  static RedisObj* Create(const ObjEncoding encoding, const DataType& val) {
    return new RedisObj(encoding, val);
  }
  const std::string& String() const;
  zset::ZSet* const ZSet() const;
  ObjEncoding Encoding() const { return encoding_; }
  void IncrRefCount() const { ++refcount_; }
  void DecrRefCount() const;

 private:
  explicit RedisObj(const ObjEncoding encoding, const DataType& val)
      : encoding_(encoding), val_(val), refcount_(1){};
  ObjEncoding encoding_;
  DataType val_;
  mutable int refcount_;
};
}  // namespace db
}  // namespace redis_simple
