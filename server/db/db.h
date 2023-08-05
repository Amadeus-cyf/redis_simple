#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "redis_obj.h"

namespace redis_simple {
namespace db {
enum DBStatus {
  dbOK = 1 << 0,
  dbErr = -1,
};

enum SetKeyFlags {
  setKeyKeepTTL = 1,
};

class RedisDb {
 public:
  static std::unique_ptr<RedisDb> init();
  const RedisObj* lookupKey(const std::string& key) const;
  DBStatus setKey(const std::string& key, const RedisObj* const val,
                  const int64_t expire) const;
  DBStatus setKey(const std::string& key, const RedisObj* const val,
                  const int64_t expire, int flags) const;
  DBStatus delKey(const std::string& key) const;
  void scanExpires(
      in_memory::Dict<std::string, int64_t>::dictScanFunc callback) const;
  double getExpiredPercentage() const {
    return dict->size() > 0 ? (double)expires->size() / dict->size() : 0.0;
  };

 private:
  RedisDb();
  bool isKeyExpired(const std::string& key) const;
  std::unique_ptr<in_memory::Dict<std::string, RedisObj*>> dict;
  std::unique_ptr<in_memory::Dict<std::string, int64_t>> expires;
  bool free_async;
  mutable int expire_cursor;
};
}  // namespace db
}  // namespace redis_simple
