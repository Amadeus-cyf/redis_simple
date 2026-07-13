#include "redis_obj.h"

#include <stdexcept>

namespace redis_simple::db {
const std::string& RedisObject::String() const {
  const auto* value = std::get_if<std::string>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not string");
  }
  return *value;
}

std::string* RedisObject::MutableString() {
  auto* value = std::get_if<std::string>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not string");
  }
  return value;
}

set::Set* RedisObject::Set() {
  auto* value = std::get_if<SetPtr>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not set");
  }
  return value->get();
}

const set::Set* RedisObject::Set() const {
  const auto* value = std::get_if<SetPtr>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not set");
  }
  return value->get();
}

list::List* RedisObject::List() {
  auto* value = std::get_if<ListPtr>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not list");
  }
  return value->get();
}

const list::List* RedisObject::List() const {
  const auto* value = std::get_if<ListPtr>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not list");
  }
  return value->get();
}

zset::ZSet* RedisObject::ZSet() {
  auto* value = std::get_if<ZSetPtr>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not zset");
  }
  return value->get();
}

const zset::ZSet* RedisObject::ZSet() const {
  const auto* value = std::get_if<ZSetPtr>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not zset");
  }
  return value->get();
}

hash::Hash* RedisObject::Hash() {
  auto* value = std::get_if<HashPtr>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not hash");
  }
  return value->get();
}

const hash::Hash* RedisObject::Hash() const {
  const auto* value = std::get_if<HashPtr>(&value_);
  if (value == nullptr) {
    throw std::invalid_argument("value type is not hash");
  }
  return value->get();
}
}  // namespace redis_simple::db
