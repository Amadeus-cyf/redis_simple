#include "db.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include "server/server.h"
#include "utils/time_utils.h"

namespace redis_simple::db {
std::unique_ptr<RedisDb> RedisDb::Create() {
  return std::unique_ptr<RedisDb>(new RedisDb());
}

RedisDb::RedisDb() : expire_cursor_(0) {
  auto hash = [](const std::string& key) {
    std::hash<std::string> hash_func;
    return hash_func(key);
  };
  auto key_compare = [](const std::string& key1, const std::string& key2) {
    return key1.compare(key2);
  };
  in_memory::Dict<std::string, RedisObjectPtr>::DictType db_type;
  db_type.hash_function = hash;
  db_type.key_dup = nullptr;
  db_type.val_dup = nullptr;
  db_type.key_destructor = nullptr;
  db_type.val_destructor = nullptr;
  db_type.key_compare = key_compare;
  dict_ = in_memory::Dict<std::string, RedisObjectPtr>::Create(db_type);

  in_memory::Dict<std::string, int64_t>::DictType expires_type;
  expires_type.hash_function = hash;
  expires_type.key_dup = nullptr;
  expires_type.val_dup = nullptr;
  expires_type.key_destructor = nullptr;
  expires_type.val_destructor = nullptr;
  expires_type.key_compare = nullptr;
  expires_ = in_memory::Dict<std::string, int64_t>::Create(expires_type);
}

const RedisObject* RedisDb::LookupKey(const std::string& key) {
  auto* const result = dict_->FindValue(key);
  if (result == nullptr) {
    return nullptr;
  }
  const RedisObject* object = result->get();
  if (IsKeyExpired(key)) {
    RS_LOG_DEBUG("look up key: key %s expired\n", key.c_str());
    // If key is already expired, delete the key and return a null pointer.
    object = nullptr;
    assert(dict_->Delete(key));
    assert(expires_->Delete(key));
  }
  return object;
}

DbStatus RedisDb::SetKey(const std::string& key, RedisObjectPtr object,
                         int64_t expire) {
  return SetKey(key, std::move(object), expire, 0);
}

DbStatus RedisDb::SetKey(const std::string& key, RedisObjectPtr object,
                         int64_t expire, int flags) {
  if (object == nullptr) {
    return DbStatus::kError;
  }
  dict_->Set(key, std::move(object));
  if (!HasFlag(flags, SetKeyFlag::kKeepTtl) && expire == 0) {
    expires_->Delete(key);
  }
  if (expire > 0) {
    expires_->Set(key, expire);
    RS_LOG_DEBUG("add expire %lld\n", expire);
  }
  return DbStatus::kOk;
}

DbStatus RedisDb::DeleteKey(const std::string& key) {
  if (!dict_->Delete(key)) {
    return DbStatus::kError;
  }
  if (expires_->Size() > 0) {
    expires_->Delete(key);
  }
  return DbStatus::kOk;
}

/*
 * Scan to delete expired keys. Return true if the scanning is not finished.
 */
bool RedisDb::ScanExpires(
    in_memory::Dict<std::string, int64_t>::DictScanFunc callback) {
  expire_cursor_ = std::max<ssize_t>(expire_cursor_, 0);
  expire_cursor_ = expires_->Scan(expire_cursor_, callback);
  return expire_cursor_ >= 0;
}

bool RedisDb::IsKeyExpired(const std::string& key) const {
  if (expires_->Size() == 0) {
    return false;
  }
  const auto result = expires_->Get(key);
  if (!result.has_value()) {
    return false;
  }
  int64_t now = utils::NowInMilliseconds();
  return *result < now;
}
}  // namespace redis_simple::db
