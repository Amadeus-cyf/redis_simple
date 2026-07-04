#include <cstddef>
#include <cstdint>
#include <optional>
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
std::optional<int64_t> ZRem(db::RedisDb* redis_db, const ZRemArgs* args);
}  // namespace

void HandleZRem(Client* const client) {
  ZRemArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = ZRem(redis_db, &args);
    client->AddReply(result.has_value()
                         ? reply::FromInt64(*result)
                         : reply::FromInt64(reply::ReplyStatus::kError));
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
  for (size_t i = 1; i < args.size(); ++i) {
    zset_args->elements.push_back(args[i]);
  }
  return 0;
}

std::optional<int64_t> ZRem(db::RedisDb* redis_db, const ZRemArgs* args) {
  if (redis_db == nullptr || args == nullptr) {
    return std::nullopt;
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    RS_LOG_DEBUG("key not found\n");
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return std::nullopt;
  }
  try {
    auto* zset = obj->ZSet();
    int64_t deleted = 0;
    for (const auto& element : args->elements) {
      deleted += zset->Delete(element) ? 1 : 0;
    }
    return deleted;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::zsets
