#include "server/commands/t_string/delete.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_string {
void DeleteCommand::exec(Client* const client) const {
  printf("delete command called\n");
  StrArgs args;
  if (parseArgs(client->getArgs(), &args) < 0) {
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->getDb().lock()) {
    if (genericDelete(db, &args) < 0) {
      client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyOK));
  } else {
    printf("db pointer expired\n");
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
  }
}

int DeleteCommand::parseArgs(const std::vector<std::string>& args,
                             StrArgs* str_args) const {
  if (args.empty()) {
    printf("invalid args\n");
    return -1;
  }
  str_args->key = args[0];
  return 0;
}

int DeleteCommand::genericDelete(std::shared_ptr<const db::RedisDb> db,
                                 const StrArgs* args) const {
  return db->delKey(args->key);
}
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
