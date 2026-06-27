#pragma once

#include <string>
#include <variant>

#include "storage/set/set.h"
#include "storage/zset/zset.h"

namespace redis_simple {
namespace db {
class RedisObject {
 private:
  using DataType = std::variant<std::string, set::Set*, zset::ZSet*>;

 public:
  enum class ObjEncoding {
    kString = 1,
    kSet = 2,
    kZSet = 3,
  };

  static RedisObject* CreateWithString(const std::string& val) {
    return Create(ObjEncoding::kString, val);
  }
  static RedisObject* CreateWithSet(set::Set* const set) {
    return Create(ObjEncoding::kSet, set);
  }
  static RedisObject* CreateWithZSet(zset::ZSet* const zset) {
    return Create(ObjEncoding::kZSet, zset);
  }
  static RedisObject* Create(const ObjEncoding encoding, const DataType& val) {
    return new RedisObject(encoding, val);
  }
  const std::string& String() const;
  set::Set* const Set() const;
  zset::ZSet* const ZSet() const;
  ObjEncoding Encoding() const { return encoding_; }
  void IncrRefCount() const { ++refcount_; }
  void DecrRefCount() const;

 private:
  explicit RedisObject(const ObjEncoding encoding, const DataType& val)
      : encoding_(encoding), val_(val), refcount_(1) {}
  ObjEncoding encoding_;
  DataType val_;
  mutable int refcount_;
};
}  // namespace db
}  // namespace redis_simple
