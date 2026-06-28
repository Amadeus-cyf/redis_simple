#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "storage/set/set.h"
#include "storage/zset/zset.h"

namespace redis_simple::db {
class RedisObject {
 private:
  using SetPtr = std::unique_ptr<set::Set>;
  using ZSetPtr = std::unique_ptr<zset::ZSet>;
  using DataType = std::variant<std::string, SetPtr, ZSetPtr>;

 public:
  enum class ObjEncoding {
    kString = 1,
    kSet = 2,
    kZSet = 3,
  };

  static std::unique_ptr<RedisObject> CreateWithString(const std::string& val) {
    return Create(ObjEncoding::kString, DataType(val));
  }
  static std::unique_ptr<RedisObject> CreateWithSet(
      std::unique_ptr<set::Set> set) {
    return Create(ObjEncoding::kSet, DataType(std::move(set)));
  }
  static std::unique_ptr<RedisObject> CreateWithZSet(
      std::unique_ptr<zset::ZSet> zset) {
    return Create(ObjEncoding::kZSet, DataType(std::move(zset)));
  }
  static std::unique_ptr<RedisObject> Create(ObjEncoding encoding,
                                             DataType val) {
    return std::unique_ptr<RedisObject>(
        new RedisObject(encoding, std::move(val)));
  }
  const std::string& String() const;
  set::Set* Set() const;
  zset::ZSet* ZSet() const;
  ObjEncoding Encoding() const { return encoding_; }

 private:
  explicit RedisObject(ObjEncoding encoding, DataType val)
      : encoding_(encoding), val_(std::move(val)) {}
  ObjEncoding encoding_;
  DataType val_;
};
}  // namespace redis_simple::db
