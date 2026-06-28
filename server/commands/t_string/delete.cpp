#include "server/commands/t_string/delete.h"

#include "server/client.h"
#include "server/commands/t_string/args.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_string {
namespace {
int ParseArgs(const std::vector<std::string>& args, StringArgs* string_args);
int Delete(db::RedisDb* redis_db, const StringArgs* args);
}  // namespace

void ExecuteDelete(Client* const client) {
  RS_LOG_DEBUG("delete command called\n");
  StringArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const int deleted = Delete(redis_db, &args);
    if (deleted < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(deleted));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args, StringArgs* string_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  string_args->key = args[0];
  return 0;
}

int Delete(db::RedisDb* redis_db, const StringArgs* args) {
  return redis_db->DeleteKey(args->key) == db::DbStatus::kOk ? 1 : 0;
}
}  // namespace
}  // namespace redis_simple::command::t_string
