#include "server/commands/t_zset/zrank.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZRankCommand::Exec(Client* const client) const {
  ZSetArgs args;
  if (ParseArgs(client->getArgs(), &args) < 0) {
    client->addReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->getDb().lock()) {
    int rank = GenericZRank(db, &args);
    if (rank < 0) {
      client->addReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->addReply(reply::FromInt64(rank));
  } else {
    printf("db pointer expired\n");
    client->addReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int ZRankCommand::ParseArgs(const std::vector<std::string>& args,
                            ZSetArgs* zset_args) const {
  if (args.size() < 2) {
    printf("invalid number of args\n");
    return -1;
  }
  const std::string& key = args[0];
  const std::string& ele = args[1];
  zset_args->key = key, zset_args->ele = ele;
  return 0;
}

int ZRankCommand::GenericZRank(std::shared_ptr<const db::RedisDb> db,
                               const ZSetArgs* args) const {
  if (!db || !args) {
    return -1;
  }
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (!obj) {
    printf("key not found\n");
    return -1;
  }
  if (obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return -1;
  }
  try {
    const zset::ZSet* const zset = obj->ZSet();
    return zset->getRank(args->ele);
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
