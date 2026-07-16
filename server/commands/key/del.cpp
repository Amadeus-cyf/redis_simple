#include <string_view>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::key {
namespace {
int DeleteKeys(db::RedisDb* const redis_db, const CommandArgs& keys) {
  int deleted = 0;
  for (std::string_view key : keys) {
    if (redis_db->DeleteKey(key) == db::DbStatus::kOk) {
      ++deleted;
    }
  }
  return deleted;
}
}  // namespace

void HandleDel(Client* const client) {
  RS_LOG_DEBUG("del command called\n");
  const auto& keys = client->Args();
  if (keys.empty()) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    client->AddReply(reply::FromInt64(DeleteKeys(redis_db, keys)));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

void HandleUnlink(Client* const client) { HandleDel(client); }
}  // namespace redis_simple::command::key
