#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::command::zsets {
namespace {
struct ZCardArgs {
  std::string_view key;
};
int ParseArgs(const CommandArgs& args, ZCardArgs* zcard_args);
std::optional<int64_t> ToReplyInteger(size_t value);
std::optional<int64_t> ZCard(db::RedisDb* redis_db, const ZCardArgs* args);
}  // namespace

void HandleZCard(Client* const client) {
  ZCardArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = ZCard(redis_db, &args);
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const CommandArgs& args, ZCardArgs* const zcard_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zcard_args->key = args[0];
  return 0;
}

std::optional<int64_t> ToReplyInteger(size_t value) {
  if (value > static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
    return std::nullopt;
  }
  return static_cast<int64_t>(value);
}

std::optional<int64_t> ZCard(db::RedisDb* redis_db, const ZCardArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
    return std::nullopt;
  }
  try {
    const auto* zset = obj->ZSet();
    return ToReplyInteger(zset->Size());
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::zsets
