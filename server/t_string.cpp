#include "t_string.h"

#include "server/client.h"
#include "server/networking/handler/write_client.h"
#include "server/redis_cmd/redis_cmd.h"
#include "server/reply/reply.h"
#include "server/t_cmd.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace t_cmd {
namespace {
struct StrArgs {
  std::string key;
  std::string val;
  int64_t expire;
};

int parseSetArgs(const RedisCommand* cmd, StrArgs* str_args) {
  if (!cmd) {
    printf("no command\n");
    return -1;
  }
  const std::vector<std::string>& args = cmd->getArgs();
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

int parseGetDelArgs(const RedisCommand* cmd, StrArgs* str_args) {
  if (!cmd) {
    printf("no command\n");
    return -1;
  }
  const std::vector<std::string>& args = cmd->getArgs();
  if (args.empty()) {
    printf("invalid args\n");
    return -1;
  }
  str_args->key = args[0];
  return 0;
}

int genericSet(const db::RedisDb* db, const StrArgs* args) {
  const db::RedisObj* val = db::RedisObj::createRedisStrObj(args->val);
  return db->setKey(args->key, val, args->expire, 0);
}

const db::RedisObj* genericGet(const db::RedisDb* db, const StrArgs* args) {
  return db->lookupKey(args->key);
}

int genericDel(const db::RedisDb* db, const StrArgs* args) {
  return db->delKey(args->key);
}
}  // namespace

void setCommand(Client* const client) {
  printf("set command called\n");
  StrArgs args;
  if (parseSetArgs(client->getCmd(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  int r = genericSet(client->getDb(), &args);
  addReplyToClient(client, reply::fromInt64(r));
}

void getCommand(Client* const client) {
  printf("get command called\n");
  StrArgs args;
  if (parseGetDelArgs(client->getCmd(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  const db::RedisObj* robj = genericGet(client->getDb(), &args);
  if (!robj) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  try {
    const std::string& s = robj->getString();
    addReplyToClient(client, reply::fromBulkString(s));
  } catch (const std::exception& e) {
    printf("catch type exception %s", e.what());
    addReplyToClient(client, reply::fromInt64(-1));
  }
}

void delCommand(Client* const client) {
  printf("delete command called\n");
  StrArgs args;
  if (parseGetDelArgs(client->getCmd(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  if (genericDel(client->getDb(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  addReplyToClient(client, reply::fromInt64(1));
}
}  // namespace t_cmd
}  // namespace redis_simple
