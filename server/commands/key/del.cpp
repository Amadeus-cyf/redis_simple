#include <string>
#include <vector>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::key {
namespace {
struct DeleteArgs {
  std::vector<std::string> keys;
};

int ParseDeleteArgs(const std::vector<std::string>& args,
                    DeleteArgs* delete_args);
int DeleteKeys(db::RedisDb* redis_db, const DeleteArgs* args);

int ParseDeleteArgs(const std::vector<std::string>& args,
                    DeleteArgs* const delete_args) {
  if (args.empty()) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  delete_args->keys = args;
  return 0;
}

int DeleteKeys(db::RedisDb* const redis_db, const DeleteArgs* const args) {
  int deleted = 0;
  for (const std::string& key : args->keys) {
    if (redis_db->DeleteKey(key) == db::DbStatus::kOk) {
      ++deleted;
    }
  }
  return deleted;
}
}  // namespace

void HandleDel(Client* const client) {
  RS_LOG_DEBUG("del command called\n");
  DeleteArgs args;
  if (ParseDeleteArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    client->AddReply(reply::FromInt64(DeleteKeys(redis_db, &args)));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}
}  // namespace redis_simple::command::key
