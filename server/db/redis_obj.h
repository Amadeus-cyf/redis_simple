#pragma once

#include <memory>
#include <string>
#include <variant>

#include "server/zset/zset.h"

namespace redis_simple {
namespace db {
class RedisObj {
 private:
  using DataType = std::variant<std::string, const zset::ZSet*>;

 public:
  enum class ObjEncoding {
    objEncodingString = 1,
    objEncodingZSet = 2,
  };

  static RedisObj* CreateString(const std::string& val) {
    return Create(ObjEncoding::objEncodingString, val);
  }
  static RedisObj* CreateZSet(const zset::ZSet* const zset) {
    return Create(ObjEncoding::objEncodingZSet, zset);
  }
  static RedisObj* Create(const ObjEncoding encoding, const DataType& val) {
    return new RedisObj(encoding, val);
  }
  const std::string& String() const;
  const zset::ZSet* const ZSet() const;
  ObjEncoding Encoding() const { return encoding; }
  void IncrRefCount() const { ++refcount; }
  void DecrRefCount() const;

 private:
  explicit RedisObj(const ObjEncoding encoding, const DataType& val)
      : encoding(encoding), val(val), refcount(1){};
  ObjEncoding encoding;
  DataType val;
  mutable int refcount;
};
}  // namespace db
}  // namespace redis_simple
