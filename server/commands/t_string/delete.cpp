#include "server/commands/t_string/delete.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_string {
void DeleteCommand::Exec(Client* const client) const {
  RS_LOG_DEBUG("delete command called\n");
  StrArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (auto db = client->DB().lock()) {
    if (Delete(db, &args) < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyOK));
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int DeleteCommand::ParseArgs(const std::vector<std::string>& args,
                             StrArgs* str_args) const {
  if (args.empty()) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  str_args->key = args[0];
  return 0;
}

int DeleteCommand::Delete(std::shared_ptr<const db::RedisDb> db,
                          const StrArgs* args) const {
  return db->DeleteKey(args->key);
}
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
