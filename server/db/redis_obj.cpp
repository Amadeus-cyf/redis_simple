#include "redis_obj.h"

namespace redis_simple {
namespace db {
const std::string& RedisObj::String() const {
  if (encoding != ObjEncoding::objEncodingString) {
    throw std::invalid_argument("value type is not string");
  }
  return std::get<std::string>(this->val);
}

const zset::ZSet* const RedisObj::ZSet() const {
  if (encoding != ObjEncoding::objEncodingZSet) {
    throw std::invalid_argument("value type is not zset");
  }
  return std::get<const zset::ZSet*>(this->val);
}

void RedisObj::DecrRefCount() const {
  if (refcount == 1) {
    // free memory based on object type
    if (encoding == ObjEncoding::objEncodingZSet) {
      delete std::get<const zset::ZSet*>(val);
    }
    delete this;
  } else {
    --refcount;
  }
}
}  // namespace db
}  // namespace redis_simple
