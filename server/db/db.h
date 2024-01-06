#pragma once

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
  static RedisDb* Init();
  const RedisObj* LookupKey(const std::string& key) const;
  DBStatus SetKey(const std::string& key, const RedisObj* const val,
                  const int64_t expire) const;
  DBStatus SetKey(const std::string& key, const RedisObj* const val,
                  const int64_t expire, int flags) const;
  DBStatus DeleteKey(const std::string& key) const;
  bool ScanExpires(
      in_memory::Dict<std::string, int64_t>::dictScanFunc callback) const;
  double ExpiredPercentage() const {
    return dict_->Size() > 0 ? (double)expires_->Size() / dict_->Size() : 0.0;
  };

 private:
  RedisDb();
  bool IsKeyExpired(const std::string& key) const;
  std::unique_ptr<in_memory::Dict<std::string, RedisObj*>> dict_;
  std::unique_ptr<in_memory::Dict<std::string, int64_t>> expires_;
  bool free_async_;
  mutable ssize_t expire_cursor_;
};
}  // namespace db
}  // namespace redis_simple
