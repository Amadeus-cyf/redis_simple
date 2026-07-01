#include <string>
#include <vector>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::key {
namespace {
struct ExistsArgs {
  std::vector<std::string> keys;
};

int ParseExistsArgs(const std::vector<std::string>& args,
                    ExistsArgs* exists_args) {
  if (args.empty()) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  exists_args->keys = args;
  return 0;
}

int CountExistingKeys(db::RedisDb* const redis_db,
                      const ExistsArgs* const args) {
  int existing = 0;
  for (const std::string& key : args->keys) {
    if (redis_db->LookupKey(key) != nullptr) {
      ++existing;
    }
  }
  return existing;
}
}  // namespace

void HandleExists(Client* const client) {
  RS_LOG_DEBUG("exists command called\n");
  ExistsArgs args;
  if (ParseExistsArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    client->AddReply(reply::FromInt64(CountExistingKeys(redis_db, &args)));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}
}  // namespace redis_simple::command::key
