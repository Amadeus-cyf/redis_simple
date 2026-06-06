#include "db.h"

#include <cassert>

#include "server/server.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace db {
RedisDb* RedisDb::Init() { return new RedisDb(); }

RedisDb::RedisDb() : free_async_(false), expire_cursor_(0) {
  auto hash = [](const std::string& key) {
    std::hash<std::string> h;
    return h(key);
  };
  auto key_compare = [](const std::string& k1, const std::string& k2) {
    return k1.compare(k2);
  };
  auto object_destructor = [this](const RedisObject* const object) {
    if (!free_async_) {
      object->DecrRefCount();
    }
  };

  in_memory::Dict<std::string, RedisObject*>::DictType db_type;
  db_type.hash_function = hash;
  db_type.key_dup = nullptr;
  db_type.val_dup = nullptr;
  db_type.key_destructor = nullptr;
  db_type.val_destructor = object_destructor;
  db_type.key_compare = key_compare;
  dict_ = in_memory::Dict<std::string, RedisObject*>::Init(db_type);

  in_memory::Dict<std::string, int64_t>::DictType expires_type;
  expires_type.hash_function = hash;
  expires_type.key_dup = nullptr;
  expires_type.val_dup = nullptr;
  expires_type.key_destructor = nullptr;
  expires_type.val_destructor = nullptr;
  expires_type.key_compare = nullptr;
  expires_ = in_memory::Dict<std::string, int64_t>::Init(expires_type);
}

const RedisObject* RedisDb::LookupKey(const std::string& key) const {
  const auto result = dict_->Get(key);
  if (!result.has_value()) return nullptr;
  const RedisObject* val = result.value();
  if (IsKeyExpired(key)) {
    RS_LOG_DEBUG("look up key: key %s expired\n", key.c_str());
    // If key is already expired, delete the key and return a null pointer.
    val = nullptr;
    assert(dict_->Delete(key));
    assert(expires_->Delete(key));
  }
  return val;
}

DbStatus RedisDb::SetKey(const std::string& key, const RedisObject* const val,
                         const int64_t expire) const {
  return SetKey(key, val, expire, 0);
}

DbStatus RedisDb::SetKey(const std::string& key, const RedisObject* const val,
                         const int64_t expire, int flags) const {
  dict_->Set(key, const_cast<RedisObject*>(val));
  if (!(flags & SetKeyFlags::kKeepTtl) && expire == 0) {
    expires_->Delete(key);
  }
  if (expire > 0) {
    expires_->Set(key, expire);
    RS_LOG_DEBUG("add expire %lld\n", expire);
  }
  val->IncrRefCount();
  return DbStatus::kOk;
}

DbStatus RedisDb::DeleteKey(const std::string& key) const {
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
    in_memory::Dict<std::string, int64_t>::DictScanFunc callback) const {
  if (expire_cursor_ < 0) {
    expire_cursor_ = 0;
  }
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
  int64_t now = utils::GetNowInMilliseconds();
  return result.value() < now;
}
}  // namespace db
}  // namespace redis_simple
