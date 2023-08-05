#include "t_string.h"

#include "server/client.h"
#include "server/networking/handler/write_client.h"
#include "server/redis_cmd/redis_cmd.h"
#include "server/reply/reply.h"
#include "server/t_cmd.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace t_cmd {
void setCommand(Client* const client) {
  printf("set command called\n");
  const db::RedisDb* db = client->getDb();
  const RedisCommand* cmd = client->getCmd();
  if (!cmd) {
    printf("no command\n");
    return;
  }
  if (cmd->getArgs().size() < 2) {
    printf("invalid args\n");
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  const std::string& key = cmd->getArgs()[0];
  int64_t expire = 0;
  if (cmd->getArgs().size() >= 3) {
    int64_t now = utils::getNowInMilliseconds();
    expire = now + std::stoll(cmd->getArgs()[2]);
  }
  const db::RedisObj* val = db::RedisObj::createRedisStrObj(cmd->getArgs()[1]);
  addReplyToClient(client, reply::fromInt64(db->setKey(key, val, expire)));
}

void getCommand(Client* const client) {
  printf("get command called\n");
  const db::RedisDb* db = client->getDb();
  const RedisCommand* cmd = client->getCmd();
  if (!cmd) {
    printf("no command\n");
    return;
  }
  if (cmd->getArgs().empty()) {
    printf("invalid args\n");
    return;
  }
  const std::string& key = cmd->getArgs()[0];
  const db::RedisObj* val_obj = db->lookupKey(key);
  if (!val_obj) {
    addReplyToClient(client, reply::fromInt64(db::dbErr));
    return;
  }
  const std::string& val = val_obj->getString();
  addReplyToClient(client, reply::fromBulkString(val));
  val_obj->decrRefCount();
}

void delCommand(Client* const client) {
  printf("delete command called\n");
  const db::RedisDb* db = client->getDb();
  const RedisCommand* cmd = client->getCmd();
  if (!cmd) {
    printf("no comman\n");
    return;
  }
  if (cmd->getArgs().empty()) {
    printf("invalid args\n");
    return;
  }
  const std::string& key = cmd->getArgs()[0];
  addReplyToClient(client, reply::fromInt64(db->delKey(key)));
}
}  // namespace t_cmd
}  // namespace redis_simple
