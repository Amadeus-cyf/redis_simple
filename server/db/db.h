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
  static std::unique_ptr<RedisDb> Init();
  const RedisObj* LookupKey(const std::string& key) const;
  DBStatus SetKey(const std::string& key, const RedisObj* const val,
                  const int64_t expire) const;
  DBStatus SetKey(const std::string& key, const RedisObj* const val,
                  const int64_t expire, int flags) const;
  DBStatus DeleteKey(const std::string& key) const;
  void ScanExpires(
      in_memory::Dict<std::string, int64_t>::dictScanFunc callback) const;
  double ExpiredPercentage() const {
    return dict->Size() > 0 ? (double)expires->Size() / dict->Size() : 0.0;
  };

 private:
  RedisDb();
  bool IsKeyExpired(const std::string& key) const;
  std::unique_ptr<in_memory::Dict<std::string, RedisObj*>> dict;
  std::unique_ptr<in_memory::Dict<std::string, int64_t>> expires;
  bool free_async;
  mutable int expire_cursor;
};
}  // namespace db
}  // namespace redis_simple
