#include "db.h"

#include <cassert>

#include "server/server.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace db {
RedisDb* RedisDb::Init() { return new RedisDb(); }

RedisDb::RedisDb() : expire_cursor_(0) {
  auto hash = [](const std::string& key) {
    std::hash<std::string> h;
    return h(key);
  };
  auto key_compare = [](const std::string& k1, const std::string& k2) {
    return k1.compare(k2);
  };
  auto robj_dtr = [this](const RedisObj* const robj) {
    if (!free_async_) {
      robj->DecrRefCount();
    }
  };

  in_memory::Dict<std::string, RedisObj*>::DictType dbType;
  dbType.hashFunction = hash;
  dbType.keyDup = nullptr;
  dbType.valDup = nullptr;
  dbType.keyDestructor = nullptr;
  dbType.valDestructor = robj_dtr;
  dbType.keyCompare = key_compare;
  dict_ = in_memory::Dict<std::string, RedisObj*>::Init(dbType);

  in_memory::Dict<std::string, int64_t>::DictType expiresType;
  expiresType.hashFunction = hash;
  expiresType.keyDup = nullptr;
  expiresType.valDup = nullptr;
  expiresType.keyDestructor = nullptr;
  expiresType.valDestructor = nullptr;
  expiresType.keyCompare = nullptr;
  expires_ = in_memory::Dict<std::string, int64_t>::Init(expiresType);
}

const RedisObj* RedisDb::LookupKey(const std::string& key) const {
  const auto opt = dict_->Get(key);
  if (!opt.has_value()) return nullptr;
  const RedisObj* val = opt.value();
  if (IsKeyExpired(key)) {
    RS_LOG_DEBUG("look up key: key %s expired\n", key.c_str());
    // If key is already expired, delete the key and return a null pointer.
    val = nullptr;
    assert(dict_->Delete(key));
    assert(expires_->Delete(key));
  }
  return val;
}

DBStatus RedisDb::SetKey(const std::string& key, const RedisObj* const val,
                         const int64_t expire) const {
  return SetKey(key, val, expire, 0);
}

DBStatus RedisDb::SetKey(const std::string& key, const RedisObj* const val,
                         const int64_t expire, int flags) const {
  dict_->Set(key, const_cast<RedisObj*>(val));
  if (!(flags & SetKeyFlags::setKeyKeepTTL) && expire == 0) {
    expires_->Delete(key);
  }
  if (expire > 0) {
    expires_->Set(key, expire);
    RS_LOG_DEBUG("add expire %lld\n", expire);
  }
  val->IncrRefCount();
  return DBStatus::dbOK;
}

DBStatus RedisDb::DeleteKey(const std::string& key) const {
  if (!dict_->Delete(key)) {
    return DBStatus::dbErr;
  }
  if (expires_->Size() > 0) {
    expires_->Delete(key);
  }
  return DBStatus::dbOK;
}

/*
 * Scan to delete expired keys. Return true if the scanning is not finished.
 */
bool RedisDb::ScanExpires(
    in_memory::Dict<std::string, int64_t>::dictScanFunc callback) const {
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
  const auto opt = expires_->Get(key);
  if (!opt.has_value()) {
    return false;
  }
  int64_t now = utils::GetNowInMilliseconds();
  return opt.value() < now;
}
}  // namespace db
}  // namespace redis_simple
