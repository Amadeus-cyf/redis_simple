#include "server/commands/t_string/set.h"

#include "server/client.h"
#include "server/reply/reply.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace command {
namespace t_string {
void SetCommand::exec(Client* const client) const {
  printf("set command called\n");
  StrArgs args;
  if (parseArgs(client->getArgs(), &args) < 0) {
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->getDb().lock()) {
    if (genericSet(db, &args) < 0) {
      client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyOK));
  } else {
    printf("db pointer expired\n");
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
  }
}

int SetCommand::parseArgs(const std::vector<std::string>& args,
                          StrArgs* str_args) const {
  if (args.size() < 2) {
    printf("invalid args\n");
    return -1;
  }
  str_args->key = args[0], str_args->val = args[1], str_args->expire = 0;
  if (args.size() >= 3) {
    int64_t now = utils::getNowInMilliseconds();
    str_args->expire = now + std::stoll(args[2]);
  }
  return 0;
}

int SetCommand::genericSet(std::shared_ptr<const db::RedisDb> db,
                           const StrArgs* args) const {
  const db::RedisObj* val = db::RedisObj::createRedisStrObj(args->val);
  int r = db->setKey(args->key, val, args->expire, 0);
  val->decrRefCount();
  return r;
}
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
