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

int parseArgs(const RedisCommand* cmd, ZSetArgs* zset_args) {
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

int genericZadd(const db::RedisDb* db, const ZSetArgs* args) {
  if (!db) {
    return -1;
  }
  const db::RedisObj* obj = db->lookupKey(args->key);
  if (obj && obj->getEncoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return -1;
  }
  if (!obj) {
    obj = db::RedisObj::createRedisZSetObj(z_set::ZSet::init());
    if (db->setKey(args->key, obj, 0) == db::DBStatus::dbErr) {
      return -1;
    }
  }
  const z_set::ZSet* const zset = obj->getZSet();
  zset->addOrUpdate(args->ele, args->score);
  return 0;
}
}  // namespace

void zaddCommand(Client* const client) {
  ZSetArgs args;
  if (parseArgs(client->getCmd(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  if (genericZadd(client->getDb(), &args) < 0) {
    addReplyToClient(client, reply::fromInt64(-1));
    return;
  }
  addReplyToClient(client, reply::fromInt64(1));
}

void zdelCommand(Client* const client) {}
}  // namespace t_cmd
}  // namespace redis_simple
