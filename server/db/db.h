#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "server/db/redis_obj.h"

namespace redis_simple::db {
using RedisObjectPtr = std::unique_ptr<RedisObject>;

enum class DbStatus {
  kOk = 1 << 0,
  kError = -1,
};

enum class SetKeyFlag {
  kKeepTtl = 1,
};

constexpr int ToInt(SetKeyFlag flag) { return static_cast<int>(flag); }
constexpr bool HasFlag(int flags, SetKeyFlag flag) {
  return (flags & ToInt(flag)) != 0;
}

class RedisDb {
 public:
  static std::unique_ptr<RedisDb> Init();
  const RedisObject* LookupKey(const std::string& key);
  DbStatus SetKey(const std::string& key, RedisObjectPtr val, int64_t expire);
  DbStatus SetKey(const std::string& key, RedisObjectPtr val, int64_t expire,
                  int flags);
  DbStatus DeleteKey(const std::string& key);
  bool ScanExpires(
      in_memory::Dict<std::string, int64_t>::DictScanFunc callback);
  double ExpiredPercentage() const {
    return dict_->Size() > 0 ? (double)expires_->Size() / dict_->Size() : 0.0;
  };

 private:
  RedisDb();
  bool IsKeyExpired(const std::string& key) const;
  std::unique_ptr<in_memory::Dict<std::string, RedisObjectPtr>> dict_;
  std::unique_ptr<in_memory::Dict<std::string, int64_t>> expires_;
  ssize_t expire_cursor_;
};
}  // namespace redis_simple::db
