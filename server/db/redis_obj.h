#pragma once

#include <memory>
#include <string>

namespace redis_simple {
namespace db {
enum class ObjType {
  objString = 1,
  objSignedInt = 2,
  objPtr = 3,
};

enum class ObjEncoding {
  objEncodingString = 1,
};

using Val = std::variant<std::string, int64_t>;

class RedisObj {
 public:
  static RedisObj* createStringRedisObj(const std::string& val) {
    return createRedisObj(ObjType::objString, val);
  }
  static RedisObj* createRedisObj(ObjType type,
                                  std::variant<std::string, int64_t> val) {
    return new RedisObj(type, val);
  }
  ObjType getType() const { return type; }
  const Val& getVal() const { return val; }
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
  explicit RedisObj(ObjType type, std::variant<std::string, int64_t> val)
      : type(type), val(val), refcount(1){};
  ObjType type;
  Val val;
  mutable int refcount;
};
}  // namespace db
}  // namespace redis_simple
