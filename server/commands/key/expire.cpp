#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

namespace redis_simple::command::key {
namespace {
struct KeyArgs {
  std::string_view key;
};

struct ExpireArgs {
  std::string_view key;
  int64_t ttl{0};
};

struct RenameArgs {
  std::string_view old_key;
  std::string_view new_key;
};

int ParseKeyArgs(const CommandArgs& args, KeyArgs* key_args) {
  if (args.size() != 1) {
    return -1;
  }
  key_args->key = args[0];
  return 0;
}

int ParseExpireArgs(const CommandArgs& args, ExpireArgs* expire_args) {
  if (args.size() != 2 || !utils::ToInt64(args[1], &expire_args->ttl)) {
    return -1;
  }
  expire_args->key = args[0];
  return 0;
}

int ParseRenameArgs(const CommandArgs& args, RenameArgs* rename_args) {
  if (args.size() != 2) {
    return -1;
  }
  rename_args->old_key = args[0];
  rename_args->new_key = args[1];
  return 0;
}

bool ExpireAtFromTtl(int64_t ttl, int64_t multiplier, int64_t now,
                     int64_t* expire) {
  if (ttl <= 0) {
    *expire = now - 1;
    return true;
  }
  if (ttl > std::numeric_limits<int64_t>::max() / multiplier) {
    return false;
  }
  const int64_t ttl_ms = ttl * multiplier;
  if (ttl_ms > std::numeric_limits<int64_t>::max() - now) {
    return false;
  }
  *expire = now + ttl_ms;
  return true;
}

void AddExpireReply(Client* const client, int64_t ttl_ms) {
  ExpireArgs args;
  if (ParseExpireArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  if (auto* redis_db = client->Db()) {
    int64_t expire = 0;
    if (!ExpireAtFromTtl(args.ttl, ttl_ms, utils::NowInMilliseconds(),
                         &expire)) {
      client->AddReply(reply::FromError("ERR invalid expire time"));
      return;
    }
    const auto status = redis_db->ExpireKeyAt(args.key, expire);
    client->AddReply(reply::FromInt64(status == db::DbStatus::kOk ? 1 : 0));
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void AddTtlReply(Client* const client, db::TtlResolution resolution) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  if (auto* redis_db = client->Db()) {
    client->AddReply(
        reply::FromInt64(redis_db->TimeToLive(args.key, resolution)));
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}
}  // namespace

void HandleExpire(Client* const client) {
  constexpr int64_t kMillisecondsPerSecond = 1000;
  AddExpireReply(client, kMillisecondsPerSecond);
}

void HandlePExpire(Client* const client) { AddExpireReply(client, 1); }

void HandleTtl(Client* const client) {
  AddTtlReply(client, db::TtlResolution::kSeconds);
}

void HandlePTtl(Client* const client) {
  AddTtlReply(client, db::TtlResolution::kMilliseconds);
}

void HandlePersist(Client* const client) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  if (auto* redis_db = client->Db()) {
    client->AddReply(reply::FromInt64(
        redis_db->PersistKey(args.key) == db::DbStatus::kOk ? 1 : 0));
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleRename(Client* const client) {
  RenameArgs args;
  if (ParseRenameArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  if (auto* redis_db = client->Db()) {
    if (redis_db->RenameKey(args.old_key, args.new_key) != db::DbStatus::kOk) {
      client->AddReply(reply::FromError("ERR no such key"));
      return;
    }
    client->AddReply(reply::FromString("OK"));
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleDbSize(Client* const client) {
  if (!client->Args().empty()) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  if (auto* redis_db = client->Db()) {
    if (redis_db->KeyCount() >
        static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
      client->AddReply(reply::FromError("ERR db size out of range"));
      return;
    }
    client->AddReply(
        reply::FromInt64(static_cast<int64_t>(redis_db->KeyCount())));
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleFlushDb(Client* const client) {
  if (!client->Args().empty()) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  if (auto* redis_db = client->Db()) {
    redis_db->Flush();
    client->AddReply(reply::FromString("OK"));
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}
}  // namespace redis_simple::command::key
