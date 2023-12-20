#include "db.h"

#include <cassert>

#include "server/server.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace db {
std::unique_ptr<RedisDb> RedisDb::Init() {
  return std::unique_ptr<RedisDb>(new RedisDb());
}

RedisDb::RedisDb()
    : dict(in_memory::Dict<std::string, RedisObj*>::Init()),
      expires(in_memory::Dict<std::string, int64_t>::Init()),
      expire_cursor(0) {
  auto hash = [](const std::string& key) {
    std::hash<std::string> h;
    return h(key);
  };
  auto key_compare = [](const std::string& k1, const std::string& k2) {
    return k1.compare(k2);
  };
  auto robj_dtr = [this](const RedisObj* const robj) {
    if (!free_async) {
      robj->DecrRefCount();
    }
  };

  dict->GetType()->keyCompare = key_compare;
  dict->GetType()->hashFunction = hash;
  dict->GetType()->keyDup = nullptr;
  dict->GetType()->keyDestructor = nullptr;
  dict->GetType()->valDup = nullptr;
  dict->GetType()->valDestructor = robj_dtr;

  expires->GetType()->keyCompare = nullptr;
  expires->GetType()->hashFunction = hash;
  expires->GetType()->keyDup = nullptr;
  expires->GetType()->keyDestructor = nullptr;
  expires->GetType()->valDup = nullptr;
  expires->GetType()->valDestructor = nullptr;
}

const RedisObj* RedisDb::LookupKey(const std::string& key) const {
  in_memory::Dict<std::string, RedisObj*>::DictEntry* de = dict->Find(key);
  if (!de) return nullptr;
  const RedisObj* val = de->val;
  if (IsKeyExpired(key)) {
    printf("look up key: key expired\n");
    /* if key is already expired, delete the key and return nullptr */
    val = nullptr;
    assert(dict->Delete(key));
    assert(expires->Delete(key));
  }
  return val;
}

DBStatus RedisDb::SetKey(const std::string& key, const RedisObj* const val,
                         const int64_t expire) const {
  return SetKey(key, val, expire, 0);
}

DBStatus RedisDb::SetKey(const std::string& key, const RedisObj* const val,
                         const int64_t expire, int flags) const {
  dict->Replace(key, const_cast<RedisObj*>(val));
  if (!(flags & SetKeyFlags::setKeyKeepTTL)) {
    expires->Delete(key);
  }
  if (expire > 0) {
    assert(expires->Add(key, expire));
    printf("add expire %lu\n", expires->Size());
  }
  val->IncrRefCount();
  return DBStatus::dbOK;
}

DBStatus RedisDb::DeleteKey(const std::string& key) const {
  if (!dict->Delete(key)) {
    return DBStatus::dbErr;
  }
  if (expires->Size() > 0) {
    expires->Delete(key);
  }
  return DBStatus::dbOK;
}

void RedisDb::ScanExpires(
    in_memory::Dict<std::string, int64_t>::dictScanFunc callback) const {
  expire_cursor = expires->Scan(expire_cursor, callback);
}

bool RedisDb::IsKeyExpired(const std::string& key) const {
  if (expires->Size() == 0) {
    return false;
  }
  const in_memory::Dict<std::string, int64_t>::DictEntry* de =
      expires->Find(key);
  int64_t now = utils::GetNowInMilliseconds();
  return de && de->val < now;
}
}  // namespace db
}  // namespace redis_simple
