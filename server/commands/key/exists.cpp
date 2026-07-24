#include <string_view>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::command::key {
namespace {
int CountExistingKeys(db::RedisDb* const redis_db, const CommandArgs& keys) {
  int existing = 0;
  for (std::string_view key : keys) {
    if (redis_db->LookupKey(key) != nullptr) {
      ++existing;
    }
  }
  return existing;
}
}  // namespace

void HandleExists(Client* const client) {
  RS_LOG_DEBUG("exists command called\n");
  const auto& keys = client->Args();
  if (keys.empty()) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    client->AddReply(reply::FromInt64(CountExistingKeys(redis_db, keys)));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}
}  // namespace redis_simple::command::key
