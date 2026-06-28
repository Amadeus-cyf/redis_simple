#include "redis_obj.h"

namespace redis_simple::db {
const std::string& RedisObject::String() const {
  if (type_ != ObjectType::kString) {
    throw std::invalid_argument("value type is not string");
  }
  return std::get<std::string>(value_);
}

set::Set* RedisObject::Set() const {
  if (type_ != ObjectType::kSet) {
    throw std::invalid_argument("value type is not set");
  }
  return std::get<SetPtr>(value_).get();
}

list::List* RedisObject::List() const {
  if (type_ != ObjectType::kList) {
    throw std::invalid_argument("value type is not list");
  }
  return std::get<ListPtr>(value_).get();
}

zset::ZSet* RedisObject::ZSet() const {
  if (type_ != ObjectType::kZSet) {
    throw std::invalid_argument("value type is not zset");
  }
  return std::get<ZSetPtr>(value_).get();
}
}  // namespace redis_simple::db
