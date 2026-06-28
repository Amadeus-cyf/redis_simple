#include <string>
#include <vector>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::zsets {
namespace {
struct ZRemArgs {
  std::string key;
  std::vector<std::string> elements;
};
int ParseArgs(const std::vector<std::string>& args, ZRemArgs* zset_args);
int ZRem(db::RedisDb* redis_db, const ZRemArgs* args);
}  // namespace

void HandleZRem(Client* const client) {
  ZRemArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    int result = ZRem(redis_db, &args);
    if (result < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(result));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args, ZRemArgs* const zset_args) {
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

int ZRem(db::RedisDb* redis_db, const ZRemArgs* args) {
  if (redis_db == nullptr || args == nullptr) {
    return -1;
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    RS_LOG_DEBUG("key not found\n");
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
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
}  // namespace
}  // namespace redis_simple::command::zsets
