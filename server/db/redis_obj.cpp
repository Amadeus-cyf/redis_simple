#include "redis_obj.h"

namespace redis_simple {
namespace db {
const std::string& RedisObj::String() const {
  if (encoding_ != ObjEncoding::objEncodingString) {
    throw std::invalid_argument("value type is not string");
  }
  return std::get<std::string>(val_);
}

set::Set* const RedisObj::Set() const {
  if (encoding_ != ObjEncoding::objEncodingSet) {
    throw std::invalid_argument("value type is not set");
  }
  return std::get<set::Set*>(val_);
}

zset::ZSet* const RedisObj::ZSet() const {
  if (encoding_ != ObjEncoding::objEncodingZSet) {
    throw std::invalid_argument("value type is not zset");
  }
  return std::get<zset::ZSet*>(val_);
}

void RedisObj::DecrRefCount() const {
  if (refcount_ == 1) {
    // Free memory based on object type.
    if (encoding_ == ObjEncoding::objEncodingZSet) {
      delete std::get<zset::ZSet*>(val_);
    } else if (encoding_ == ObjEncoding::objEncodingSet) {
      delete std::get<set::Set*>(val_);
    }
    // Free the object.
    delete this;
  } else {
    --refcount_;
  }
}
}  // namespace db
}  // namespace redis_simple
