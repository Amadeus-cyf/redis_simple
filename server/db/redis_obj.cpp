#include "redis_obj.h"

namespace redis_simple::db {
const std::string& RedisObject::String() const {
  if (encoding_ != ObjEncoding::kString) {
    throw std::invalid_argument("value type is not string");
  }
  return std::get<std::string>(val_);
}

set::Set* RedisObject::Set() const {
  if (encoding_ != ObjEncoding::kSet) {
    throw std::invalid_argument("value type is not set");
  }
  return std::get<SetPtr>(val_).get();
}

zset::ZSet* RedisObject::ZSet() const {
  if (encoding_ != ObjEncoding::kZSet) {
    throw std::invalid_argument("value type is not zset");
  }
  return std::get<ZSetPtr>(val_).get();
}
}  // namespace redis_simple::db
