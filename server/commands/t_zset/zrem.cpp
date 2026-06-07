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
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    int r = ZRem(db, &args);
    if (r < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(r));
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

int ZRemCommand::ParseArgs(const std::vector<std::string>& args,
                           ZRemArgs* const zset_args) const {
  if (args.size() < 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zset_args->key = args[0];
  for (int i = 1; i < args.size(); ++i) {
    zset_args->elements.push_back(args[i]);
  }
  return 0;
}

int ZRemCommand::ZRem(std::shared_ptr<db::RedisDb> db,
                      const ZRemArgs* args) const {
  if (!db || !args) {
    return -1;
  }
  const auto* obj = db->LookupKey(args->key);
  if (!obj) {
    RS_LOG_DEBUG("key not found\n");
    return 0;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return -1;
  }
  try {
    auto* zset = obj->ZSet();
    int deleted = 0;
    for (const auto& element : args->elements) {
      deleted += zset->Delete(element) ? 1 : 0;
    }
    return deleted;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
