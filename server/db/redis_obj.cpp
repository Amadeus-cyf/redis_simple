#include "redis_obj.h"

namespace redis_simple {
namespace db {
const std::string& RedisObj::String() const {
  if (encoding_ != ObjEncoding::objEncodingString) {
    throw std::invalid_argument("value type is not string");
  }
  return std::get<std::string>(this->val_);
}

zset::ZSet* const RedisObj::ZSet() const {
  if (encoding_ != ObjEncoding::objEncodingZSet) {
    throw std::invalid_argument("value type is not zset");
  }
  return std::get<zset::ZSet*>(this->val_);
}

void RedisObj::DecrRefCount() const {
  if (refcount_ == 1) {
    // free memory based on object type
    if (encoding_ == ObjEncoding::objEncodingZSet) {
      delete std::get<zset::ZSet*>(val_);
    }
    delete this;
  } else {
    --refcount_;
  }
}
}  // namespace db
}  // namespace redis_simple
