#pragma once

#include <memory>
#include <string>
#include <variant>

#include "server/zset/z_set.h"

namespace redis_simple {
namespace db {
class RedisObj {
 private:
  using DataType = std::variant<std::string, const z_set::ZSet*>;

 public:
  enum class ObjEncoding {
    objEncodingString = 1,
    objEncodingZSet = 2,
  };
  static RedisObj* createRedisStrObj(const std::string& val) {
    return createRedisObj(ObjEncoding::objEncodingString, val);
  }
  static RedisObj* createRedisZSetObj(const z_set::ZSet* const zset) {
    return createRedisObj(ObjEncoding::objEncodingZSet, zset);
  }
  static RedisObj* createRedisObj(const ObjEncoding encoding,
                                  const DataType& val) {
    return new RedisObj(encoding, val);
  }
  const std::string& getString() const {
    if (encoding != ObjEncoding::objEncodingString) {
      throw std::invalid_argument("value type is not string");
    }
    return std::get<std::string>(getVal());
  }
  const z_set::ZSet* const getZSet() const {
    if (encoding != ObjEncoding::objEncodingZSet) {
      throw std::invalid_argument("value type is not zset");
    }
    return std::get<const z_set::ZSet*>(getVal());
  }
  ObjEncoding getEncoding() const { return encoding; }
  void incrRefCount() const { ++refcount; }
  void decrRefCount() const {
    if (refcount == 1) {
      // TODO: free memory based on object type
      delete this;
    } else {
      --refcount;
    }
  }

 private:
  explicit RedisObj(const ObjEncoding encoding, const DataType& val)
      : encoding(encoding), val(val), refcount(1){};
  const DataType& getVal() const { return val; }
  ObjEncoding encoding;
  DataType val;
  mutable int refcount;
};
}  // namespace db
}  // namespace redis_simple
