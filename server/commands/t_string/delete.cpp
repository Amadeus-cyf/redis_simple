#include "server/commands/t_string/delete.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_string {
void DeleteCommand::Exec(Client* const client) const {
  RS_LOG_DEBUG("delete command called\n");
  StringArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    const int deleted = Delete(db, &args);
    if (deleted < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(deleted));
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

int DeleteCommand::ParseArgs(const std::vector<std::string>& args,
                             StringArgs* string_args) const {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  string_args->key = args[0];
  return 0;
}

int DeleteCommand::Delete(std::shared_ptr<db::RedisDb> db,
                          const StringArgs* args) const {
  return db->DeleteKey(args->key) == db::DbStatus::kOk ? 1 : 0;
}
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
