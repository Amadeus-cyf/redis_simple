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

  static RedisObj* createRedisStrObj(const std::string& val) {
    return createRedisObj(ObjEncoding::objEncodingString, val);
  }
  static RedisObj* createRedisZSetObj(const zset::ZSet* const zset) {
    return createRedisObj(ObjEncoding::objEncodingZSet, zset);
  }
  static RedisObj* createRedisObj(const ObjEncoding encoding,
                                  const DataType& val) {
    return new RedisObj(encoding, val);
  }
  const std::string& getString() const;
  const zset::ZSet* const getZSet() const;
  ObjEncoding getEncoding() const { return encoding; }
  void incrRefCount() const { ++refcount; }
  void decrRefCount() const;

 private:
  explicit RedisObj(const ObjEncoding encoding, const DataType& val)
      : encoding(encoding), val(val), refcount(1){};
  ObjEncoding encoding;
  DataType val;
  mutable int refcount;
};
}  // namespace db
}  // namespace redis_simple
