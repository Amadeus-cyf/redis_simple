#include "server/commands/t_zset/zcard.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZCardCommand::Exec(Client* const client) const {
  ZAddArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    ssize_t r = ZCard(db, &args);
    if (r < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->AddReply(reply::FromInt64(r));
  } else {
    printf("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int ZCardCommand::ParseArgs(const std::vector<std::string>& args,
                            ZAddArgs* const sadd_args) const {
  if (args.size() < 1) {
    printf("invalid number of args\n");
    return -1;
  }
  sadd_args->key = std::move(args[0]);
  return 0;
}

ssize_t ZCardCommand::ZCard(std::shared_ptr<const db::RedisDb> db,
                            const ZAddArgs* args) const {
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (!obj || obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    return -1;
  }
  try {
    const zset::ZSet* zset = obj->ZSet();
    return zset->Size();
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
