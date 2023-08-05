#include "db.h"

#include "server/server.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace db {
std::unique_ptr<RedisDb> RedisDb::init() {
  return std::unique_ptr<RedisDb>(new RedisDb());
}

RedisDb::RedisDb()
    : dict(in_memory::Dict<std::string, RedisObj*>::init()),
      expires(in_memory::Dict<std::string, int64_t>::init()),
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
      robj->decrRefCount();
    }
  };

  dict->getType()->keyCompare = key_compare;
  dict->getType()->hashFunction = hash;
  dict->getType()->keyDup = nullptr;
  dict->getType()->keyDestructor = nullptr;
  dict->getType()->valDup = nullptr;
  dict->getType()->valDestructor = robj_dtr;

  expires->getType()->keyCompare = nullptr;
  expires->getType()->hashFunction = hash;
  expires->getType()->keyDup = nullptr;
  expires->getType()->keyDestructor = nullptr;
  expires->getType()->valDup = nullptr;
  expires->getType()->valDestructor = nullptr;
}

const RedisObj* RedisDb::lookupKey(const std::string& key) const {
  in_memory::Dict<std::string, RedisObj*>::DictEntry* de = dict->find(key);
  if (!de) return nullptr;
  const RedisObj* val = de->val;
  if (isKeyExpired(key)) {
    printf("look up key: key expired\n");
    /* if key is already expired, delete the key and return nullptr */
    val = nullptr;
    assert(dict->del(key) == in_memory::DictStatus::dictOK);
    assert(expires->del(key) == in_memory::DictStatus::dictOK);
  }
  return val;
}

DBStatus RedisDb::setKey(const std::string& key, const RedisObj* const val,
                         const int64_t expire) const {
  return setKey(key, val, expire, 0);
}

DBStatus RedisDb::setKey(const std::string& key, const RedisObj* const val,
                         const int64_t expire, int flags) const {
  if (dict->replace(key, const_cast<RedisObj*>(val)) !=
      in_memory::DictStatus::dictOK) {
    return DBStatus::dbErr;
  }
  if (!(flags & SetKeyFlags::setKeyKeepTTL)) {
    expires->del(key);
  }
  if (expire > 0) {
    assert(expires->add(key, expire) == in_memory::DictStatus::dictOK);
    printf("add expire %lu\n", expires->size());
  }
  val->incrRefCount();
  return DBStatus::dbOK;
}

DBStatus RedisDb::delKey(const std::string& key) const {
  in_memory::DictStatus status = dict->del(key);
  if (status == in_memory::DictStatus::dictErr) {
    return DBStatus::dbErr;
  }
  if (expires->size() > 0) {
    expires->del(key);
  }
  return DBStatus::dbOK;
}

void RedisDb::scanExpires(
    in_memory::Dict<std::string, int64_t>::dictScanFunc callback) const {
  expire_cursor = expires->scan(expire_cursor, callback);
}

bool RedisDb::isKeyExpired(const std::string& key) const {
  if (expires->size() == 0) {
    return false;
  }
  const in_memory::Dict<std::string, int64_t>::DictEntry* de =
      expires->find(key);
  int64_t now = utils::getNowInMilliseconds();
  return de && de->val < now;
}
}  // namespace db
}  // namespace redis_simple
