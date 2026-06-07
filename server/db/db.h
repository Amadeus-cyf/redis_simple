#pragma once

#include <string>

#include "memory/dict.h"
#include "server/db/redis_obj.h"

namespace redis_simple {
namespace db {
enum DbStatus {
  kOk = 1 << 0,
  kError = -1,
};

enum SetKeyFlags {
  kKeepTtl = 1,
};

class RedisDb {
 public:
  static RedisDb* Init();
  const RedisObject* LookupKey(const std::string& key);
  DbStatus SetKey(const std::string& key, const RedisObject* const val,
                  const int64_t expire);
  DbStatus SetKey(const std::string& key, const RedisObject* const val,
                  const int64_t expire, int flags);
  DbStatus DeleteKey(const std::string& key);
  bool ScanExpires(
      in_memory::Dict<std::string, int64_t>::DictScanFunc callback);
  double ExpiredPercentage() const {
    return dict_->Size() > 0 ? (double)expires_->Size() / dict_->Size() : 0.0;
  };

 private:
  RedisDb();
  bool IsKeyExpired(const std::string& key) const;
  std::unique_ptr<in_memory::Dict<std::string, const RedisObject*>> dict_;
  std::unique_ptr<in_memory::Dict<std::string, int64_t>> expires_;
  bool free_async_;
  ssize_t expire_cursor_;
};
}  // namespace db
}  // namespace redis_simple
