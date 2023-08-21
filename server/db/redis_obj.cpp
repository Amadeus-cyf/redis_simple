#include "redis_obj.h"

namespace redis_simple {
namespace db {
const std::string& RedisObj::getString() const {
  if (encoding != ObjEncoding::objEncodingString) {
    throw std::invalid_argument("value type is not string");
  }
  return std::get<std::string>(this->val);
}

const z_set::ZSet* const RedisObj::getZSet() const {
  if (encoding != ObjEncoding::objEncodingZSet) {
    throw std::invalid_argument("value type is not zset");
  }
  return std::get<const z_set::ZSet*>(this->val);
}

void RedisObj::decrRefCount() const {
  if (refcount == 1) {
    // free memory based on object type
    if (encoding == ObjEncoding::objEncodingZSet) {
      delete std::get<const z_set::ZSet*>(val);
    }
    delete this;
  } else {
    --refcount;
  }
}
}  // namespace db
}  // namespace redis_simple
