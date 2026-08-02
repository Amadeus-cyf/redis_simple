#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "memory/dict.h"
#include "server/db/async_reclaimer.h"
#include "server/db/redis_obj.h"

namespace redis_simple::aof {
class Aof;
}

namespace redis_simple::db {
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
  DbStatus UnlinkKey(std::string_view key);
  DbStatus ExpireKeyAt(std::string_view key, int64_t expire);
  DbStatus PersistKey(std::string_view key);
  DbStatus RenameKey(std::string_view old_key, std::string_view new_key);
  std::optional<int64_t> Expiration(std::string_view key) const;
  int64_t TimeToLive(std::string_view key, TtlResolution resolution);
  size_t KeyCount() const { return dict_->Size(); }
  // Key views remain valid until the database is mutated.
  template <typename Visitor>
  size_t ScanKeys(size_t cursor, size_t bucket_count, Visitor&& visitor);
  void Flush();
  ExpireSampleResult ExpireSome(size_t max_samples, int64_t now);

 private:
  friend class aof::Aof;
  RedisDb();
  void SetLoading(bool loading) { loading_ = loading; }
  bool IsKeyExpired(std::string_view key) const;
  std::unique_ptr<in_memory::Dict<std::string, RedisObjectPtr>> dict_;
  std::unique_ptr<in_memory::Dict<std::string, int64_t>> expires_;
  AsyncReclaimer reclaimer_;
  size_t expire_cursor_{};
  // Replay defers expiration checks until all historical writes are applied.
  bool loading_{};
};

template <typename Visitor>
size_t RedisDb::ScanKeys(size_t cursor, size_t bucket_count,
                         Visitor&& visitor) {
  if (bucket_count == 0 || dict_->Size() == 0) {
    return 0;
  }

  std::optional<size_t> next_cursor = cursor;
  for (size_t scanned = 0; scanned < bucket_count && next_cursor.has_value();
       ++scanned) {
    next_cursor = dict_->Scan(
        *next_cursor,
        [this, &visitor](const std::string& key, const RedisObjectPtr&) {
          if (!IsKeyExpired(key)) {
            visitor(std::string_view(key));
          }
        });
  }
  return next_cursor.value_or(0);
}
}  // namespace redis_simple::db
