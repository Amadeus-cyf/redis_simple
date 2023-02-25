#pragma once

#include <memory>
#include <string>

#include "redis_obj.h"
#include "src/memory/dict.h"

namespace redis_simple {
namespace db {
enum DBStatus {
  dbOK = 0,
  dbErr = -1,
};

enum SetKeyFlags {
  setKeyKeepTTL = 1,
};

class RedisDb {
 public:
  static std::unique_ptr<RedisDb> initDb();
  const RedisObj* lookupKey(const std::string& key) const;
  DBStatus setKey(const std::string& key, const RedisObj* const val,
                  const uint64_t expire) const;
  DBStatus setKey(const std::string& key, const RedisObj* const val,
                  const uint64_t expire, int flags) const;
  DBStatus delKey(const std::string& key) const;
  void scanExpires(
      in_memory::Dict<std::string, uint64_t>::dictScanFunc callback) const;
  double getExpiredPercentage() const {
    return dict->size() > 0 ? (double)expires->size() / dict->size() : 0.0;
  };

 private:
  RedisDb();
  bool isKeyExpired(const std::string& key) const;
  std::unique_ptr<in_memory::Dict<std::string, RedisObj*>> dict;
  std::unique_ptr<in_memory::Dict<std::string, uint64_t>> expires;
  bool free_async;
  mutable int expire_cursor;
};
}  // namespace db
}  // namespace redis_simple
