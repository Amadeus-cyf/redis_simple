#include "server/commands/t_zset//zadd.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZAddCommand::exec(Client* const client) const {
  ZSetArgs args;
  if (parseArgs(client->getArgs(), &args) < 0) {
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (genericZAdd(client->getDb(), &args) < 0) {
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  client->addReply(reply::fromInt64(reply::ReplyStatus::replyOK));
}

int ZAddCommand::parseArgs(const std::vector<std::string>& args,
                           ZSetArgs* zset_args) const {
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

int ZAddCommand::genericZAdd(const db::RedisDb* db,
                             const ZSetArgs* args) const {
  if (!db || !args) {
    return -1;
  }
  const db::RedisObj* obj = db->lookupKey(args->key);
  if (obj && obj->getEncoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return -1;
  }
  if (!obj) {
    obj = db::RedisObj::createRedisZSetObj(zset::ZSet::init());
    int r = db->setKey(args->key, obj, 0) == db::DBStatus::dbErr;
    obj->decrRefCount();
    if (r < 0) {
      return -1;
    }
  }
  try {
    const zset::ZSet* const zset = obj->getZSet();
    zset->addOrUpdate(args->ele, args->score);
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
  return 0;
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
