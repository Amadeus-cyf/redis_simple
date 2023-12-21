#include "db.h"

#include <cassert>

#include "server/server.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace db {
RedisDb* RedisDb::Init() { return new RedisDb(); }

RedisDb::RedisDb()
    : dict_(in_memory::Dict<std::string, RedisObj*>::Init()),
      expires_(in_memory::Dict<std::string, int64_t>::Init()),
      expire_cursor_(0) {
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

  dict_->GetType()->keyCompare = key_compare;
  dict_->GetType()->hashFunction = hash;
  dict_->GetType()->keyDup = nullptr;
  dict_->GetType()->keyDestructor = nullptr;
  dict_->GetType()->valDup = nullptr;
  dict_->GetType()->valDestructor = robj_dtr;

  expires_->GetType()->keyCompare = nullptr;
  expires_->GetType()->hashFunction = hash;
  expires_->GetType()->keyDup = nullptr;
  expires_->GetType()->keyDestructor = nullptr;
  expires_->GetType()->valDup = nullptr;
  expires_->GetType()->valDestructor = nullptr;
}

const RedisObj* RedisDb::LookupKey(const std::string& key) const {
  in_memory::Dict<std::string, RedisObj*>::DictEntry* de = dict_->Find(key);
  if (!de) return nullptr;
  const RedisObj* val = de->val;
  if (IsKeyExpired(key)) {
    printf("look up key: key expired\n");
    /* if key is already expired, delete the key and return nullptr */
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
  dict_->Replace(key, const_cast<RedisObj*>(val));
  if (!(flags & SetKeyFlags::setKeyKeepTTL)) {
    expires_->Delete(key);
  }
  if (expire > 0) {
    assert(expires_->Add(key, expire));
    printf("add expire %lu\n", expires_->Size());
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

void RedisDb::ScanExpires(
    in_memory::Dict<std::string, int64_t>::dictScanFunc callback) const {
  expire_cursor_ = expires_->Scan(expire_cursor_, callback);
}

bool RedisDb::IsKeyExpired(const std::string& key) const {
  if (expires_->Size() == 0) {
    return false;
  }
  const in_memory::Dict<std::string, int64_t>::DictEntry* de =
      expires_->Find(key);
  int64_t now = utils::GetNowInMilliseconds();
  return de && de->val < now;
}
}  // namespace db
}  // namespace redis_simple
