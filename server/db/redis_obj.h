#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "data_types/hash/hash.h"
#include "data_types/list/list.h"
#include "data_types/set/set.h"
#include "data_types/zset/zset.h"

namespace redis_simple::db {
class RedisObject {
 private:
  using SetPtr = std::unique_ptr<set::Set>;
  using ListPtr = std::unique_ptr<list::List>;
  using ZSetPtr = std::unique_ptr<zset::ZSet>;
  using HashPtr = std::unique_ptr<hash::Hash>;
  using Value = std::variant<std::string, SetPtr, ListPtr, ZSetPtr, HashPtr>;

 public:
  enum class ObjectType {
    kString = 1,
    kSet = 2,
    kList = 3,
    kZSet = 4,
    kHash = 5,
  };

  static std::unique_ptr<RedisObject> CreateWithString(std::string value) {
    return Create(ObjectType::kString, Value(std::move(value)));
  }
  static std::unique_ptr<RedisObject> CreateWithSet(
      std::unique_ptr<set::Set> set) {
    return Create(ObjectType::kSet, Value(std::move(set)));
  }
  static std::unique_ptr<RedisObject> CreateWithList(
      std::unique_ptr<list::List> list) {
    return Create(ObjectType::kList, Value(std::move(list)));
  }
  static std::unique_ptr<RedisObject> CreateWithZSet(
      std::unique_ptr<zset::ZSet> zset) {
    return Create(ObjectType::kZSet, Value(std::move(zset)));
  }
  static std::unique_ptr<RedisObject> CreateWithHash(
      std::unique_ptr<hash::Hash> hash) {
    return Create(ObjectType::kHash, Value(std::move(hash)));
  }
  static std::unique_ptr<RedisObject> Create(ObjectType type, Value value) {
    return std::unique_ptr<RedisObject>(
        new RedisObject(type, std::move(value)));
  }
  const std::string& String() const;
  std::string* MutableString();
  set::Set* Set() const;
  list::List* List() const;
  zset::ZSet* ZSet() const;
  hash::Hash* Hash() const;
  ObjectType Type() const { return type_; }

 private:
  explicit RedisObject(ObjectType type, Value value)
      : type_(type), value_(std::move(value)) {}
  ObjectType type_;
  Value value_;
};
}  // namespace redis_simple::db
