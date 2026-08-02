#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::command::zsets {
namespace {
std::optional<int64_t> ZRem(db::RedisDb* redis_db, const CommandArgs& args);
}  // namespace

void HandleZRem(Client* const client) {
  const auto& args = client->Args();
  if (args.size() < 2) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = ZRem(redis_db, args);
    if (result.has_value() && *result > 0) {
      client->MarkModified();
    }
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

std::optional<int64_t> ZRem(db::RedisDb* redis_db, const CommandArgs& args) {
  if (redis_db == nullptr) {
    return std::nullopt;
  }
  auto* obj = redis_db->MutableLookupKey(args[0]);
  if (obj == nullptr) {
    RS_LOG_DEBUG("key not found\n");
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return std::nullopt;
  }
  try {
    auto* zset = obj->ZSet();
    int64_t deleted = 0;
    for (size_t i = 1; i < args.size(); ++i) {
      deleted += zset->Delete(args[i]) ? 1 : 0;
    }
    return deleted;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::zsets
