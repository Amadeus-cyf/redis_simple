#include <string_view>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::command::key {
namespace {
using DeleteOperation = db::DbStatus (db::RedisDb::*)(std::string_view);

int DeleteKeys(db::RedisDb* const redis_db, const CommandArgs& keys,
               DeleteOperation operation) {
  int deleted = 0;
  for (std::string_view key : keys) {
    if ((redis_db->*operation)(key) == db::DbStatus::kOk) {
      ++deleted;
    }
  }
  return deleted;
}

void AddDeleteReply(Client* const client, DeleteOperation operation) {
  const auto& keys = client->Args();
  if (keys.empty()) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    client->AddReply(reply::FromInt64(DeleteKeys(redis_db, keys, operation)));
    return;
  }
  RS_LOG_DEBUG("db unavailable\n");
  client->AddReply(reply::FromError("ERR db unavailable"));
}
}  // namespace

void HandleDel(Client* const client) {
  RS_LOG_DEBUG("del command called\n");
  AddDeleteReply(client, &db::RedisDb::DeleteKey);
}

void HandleUnlink(Client* const client) {
  RS_LOG_DEBUG("unlink command called\n");
  AddDeleteReply(client, &db::RedisDb::UnlinkKey);
}
}  // namespace redis_simple::command::key
