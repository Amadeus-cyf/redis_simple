#include "server/commands/t_zset//zadd.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZAddCommand::Exec(Client* const client) const {
  ZSetArgs args;
  if (ParseArgs(client->getArgs(), &args) < 0) {
    client->addReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->getDb().lock()) {
    if (GenericZAdd(db, &args) < 0) {
      client->addReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->addReply(reply::FromInt64(reply::ReplyStatus::replyOK));
  } else {
    printf("db pointer expired\n");
    client->addReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int ZAddCommand::ParseArgs(const std::vector<std::string>& args,
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

int ZAddCommand::GenericZAdd(std::shared_ptr<const db::RedisDb> db,
                             const ZSetArgs* args) const {
  if (!db || !args) {
    return -1;
  }
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (obj && obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return -1;
  }
  if (!obj) {
    obj = db::RedisObj::CreateZSet(zset::ZSet::init());
    int r = db->SetKey(args->key, obj, 0) == db::DBStatus::dbErr;
    obj->DecrRefCount();
    if (r < 0) {
      return -1;
    }
  }
  try {
    const zset::ZSet* const zset = obj->ZSet();
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
