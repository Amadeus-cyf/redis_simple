#include "t_zset.h"

#include <vector>

#include "server/client.h"
#include "server/reply/reply.h"
#include "server/t_cmd.h"
#include "server/zset/z_set.h"

namespace redis_simple {
namespace t_cmd {
namespace {
struct ZSetArgs {
  std::string key;
  std::string ele;
  double score;
};

int parseZAddArgs(const RedisCommand* cmd, ZSetArgs* zset_args) {
  if (!cmd) {
    return -1;
  }
  const std::vector<std::string>& args = cmd->getArgs();
  if (args.size() < 3) {
    printf("invalid number args\n");
    return -1;
  }
  const std::string& key = args[0];
  const std::string& ele = args[1];
  double score = 0.0;
  try {
    score = stod(args[2]);
  } catch (const std::exception&) {
    printf("invalid args format\n");
    return -1;
  }
  zset_args->key = key, zset_args->ele = ele, zset_args->score = score;
  return 0;
}

int parseZRemZRankArgs(const RedisCommand* cmd, ZSetArgs* zset_args) {
  if (!cmd) {
    return -1;
  }
  const std::vector<std::string>& args = cmd->getArgs();
  if (args.size() < 2) {
    printf("invalid number of args\n");
    return -1;
  }
  const std::string& key = args[0];
  const std::string& ele = args[1];
  zset_args->key = key, zset_args->ele = ele;
  return 0;
}

int genericZAdd(const db::RedisDb* db, const ZSetArgs* args) {
  if (!db || !args) {
    return -1;
  }
  const db::RedisObj* obj = db->lookupKey(args->key);
  if (obj && obj->getEncoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return -1;
  }
  if (!obj) {
    obj = db::RedisObj::createRedisZSetObj(z_set::ZSet::init());
    int r = db->setKey(args->key, obj, 0) == db::DBStatus::dbErr;
    obj->decrRefCount();
    if (r < 0) {
      return -1;
    }
  }
  try {
    const z_set::ZSet* const zset = obj->getZSet();
    zset->addOrUpdate(args->ele, args->score);
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
  return 0;
}

int genericZRem(const db::RedisDb* db, const ZSetArgs* args) {
  if (!db || !args) {
    return -1;
  }
  const db::RedisObj* obj = db->lookupKey(args->key);
  if (!obj) {
    printf("key not found\n");
    return -1;
  }
  if (obj && obj->getEncoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return -1;
  }
  try {
    const z_set::ZSet* const zset = obj->getZSet();
    return zset->remove(args->ele) ? 0 : -1;
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}

int genericZRank(const db::RedisDb* db, const ZSetArgs* args) {
  if (!db || !args) {
    return -1;
  }
  const db::RedisObj* obj = db->lookupKey(args->key);
  if (!obj) {
    printf("key not found\n");
    return -1;
  }
  if (obj && obj->getEncoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return -1;
  }
  try {
    const z_set::ZSet* const zset = obj->getZSet();
    return zset->getRank(args->ele);
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace

void zAddCommand(Client* const client) {
  ZSetArgs args;
  if (parseZAddArgs(client->getCmd(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  if (genericZAdd(client->getDb(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  addReplyToClient(client, reply::fromInt64(1));
}

void zRemCommand(Client* const client) {
  ZSetArgs args;
  if (parseZRemZRankArgs(client->getCmd(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  if (genericZRem(client->getDb(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  addReplyToClient(client, reply::fromInt64(1));
}

void zRankCommand(Client* const client) {
  ZSetArgs args;
  if (parseZRemZRankArgs(client->getCmd(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  int rank = genericZRank(client->getDb(), &args);
  if (rank < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  addReplyToClient(client, reply::fromInt64(rank));
}
}  // namespace t_cmd
}  // namespace redis_simple
