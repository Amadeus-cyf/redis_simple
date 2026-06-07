#include "redis_obj.h"

namespace redis_simple {
namespace db {
const std::string& RedisObject::String() const {
  if (encoding_ != ObjEncoding::kString) {
    throw std::invalid_argument("value type is not string");
  }
  return std::get<std::string>(val_);
}

set::Set* const RedisObject::Set() const {
  if (encoding_ != ObjEncoding::kSet) {
    throw std::invalid_argument("value type is not set");
  }
  return std::get<set::Set*>(val_);
}

zset::ZSet* const RedisObject::ZSet() const {
  if (encoding_ != ObjEncoding::kZSet) {
    throw std::invalid_argument("value type is not zset");
  }
  return std::get<zset::ZSet*>(val_);
}

void RedisObject::DecrRefCount() const {
  if (refcount_ == 1) {
    if (encoding_ == ObjEncoding::kZSet) {
      delete std::get<zset::ZSet*>(val_);
    } else if (encoding_ == ObjEncoding::kSet) {
      delete std::get<set::Set*>(val_);
    }
    delete this;
  } else {
    --refcount_;
  }
}
}  // namespace db
}  // namespace redis_simple
