#pragma once

#include <memory>
#include <string>
#include <string_view>

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

enum class TtlResolution {
  kSeconds,
  kMilliseconds,
};

struct ExpireSampleResult {
  size_t sampled{};
  size_t expired{};
};

constexpr int ToInt(SetKeyFlag flag) { return static_cast<int>(flag); }
constexpr bool HasFlag(int flags, SetKeyFlag flag) {
  return (flags & ToInt(flag)) != 0;
}

class RedisDb {
 public:
  static std::unique_ptr<RedisDb> Create();
  const RedisObject* LookupKey(std::string_view key);
  RedisObject* MutableLookupKey(std::string_view key);
  DbStatus SetKey(std::string_view key, RedisObjectPtr object, int64_t expire);
  DbStatus SetKey(std::string_view key, RedisObjectPtr object, int64_t expire,
                  int flags);
  DbStatus DeleteKey(std::string_view key);
  DbStatus ExpireKeyAt(std::string_view key, int64_t expire);
  DbStatus PersistKey(std::string_view key);
  DbStatus RenameKey(std::string_view old_key, std::string_view new_key);
  int64_t TimeToLive(std::string_view key, TtlResolution resolution);
  size_t KeyCount() const { return dict_->Size(); }
  void Flush();
  ExpireSampleResult ExpireSome(size_t max_samples, int64_t now);

 private:
  RedisDb();
  bool IsKeyExpired(std::string_view key) const;
  std::unique_ptr<in_memory::Dict<std::string, RedisObjectPtr>> dict_;
  std::unique_ptr<in_memory::Dict<std::string, int64_t>> expires_;
  size_t expire_cursor_{};
};
}  // namespace redis_simple::db
