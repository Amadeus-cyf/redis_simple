#include "server/commands/t_zset/zrem.h"

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZRemCommand::Exec(Client* const client) const {
  ZRemArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    int r = ZRem(db, &args);
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

int ZRemCommand::ParseArgs(const std::vector<std::string>& args,
                           ZRemArgs* const zset_args) const {
  if (args.size() < 2) {
    printf("invalid number of args\n");
    return -1;
  }
  const std::string& key = args[0];
  zset_args->key = key;
  for (int i = 1; i < args.size(); ++i) {
    zset_args->elements.push_back(std::move(args[i]));
  }
  return 0;
}

int ZRemCommand::ZRem(std::shared_ptr<const db::RedisDb> db,
                      const ZRemArgs* args) const {
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
    zset::ZSet* zset = obj->ZSet();
    int deleted = 0;
    for (const std::string& element : args->elements) {
      deleted += zset->Delete(element) ? 1 : 0;
    }
    return deleted;
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
