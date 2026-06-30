#include <limits>
#include <optional>
#include <string>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::zsets {
namespace {
struct ZRankArgs {
  std::string key;
  std::string element;
};
int ParseArgs(const std::vector<std::string>& args, ZRankArgs* zset_args);
std::optional<size_t> ZRank(db::RedisDb* redis_db, const ZRankArgs* args);
}  // namespace

void HandleZRank(Client* const client) {
  ZRankArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto opt_rank = ZRank(redis_db, &args);
    if (opt_rank.has_value()) {
      if (*opt_rank >
          static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
        client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
        return;
      }
      client->AddReply(reply::FromInt64(static_cast<int64_t>(*opt_rank)));
    } else {
      client->AddReply(reply::Null());
    }
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args,
              ZRankArgs* const zset_args) {
  if (args.size() != 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zset_args->key = args[0];
  zset_args->element = args[1];
  return 0;
}

std::optional<size_t> ZRank(db::RedisDb* redis_db, const ZRankArgs* args) {
  if (redis_db == nullptr || args == nullptr) {
    return std::nullopt;
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    RS_LOG_DEBUG("key not found\n");
    return std::nullopt;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return std::nullopt;
  }
  try {
    const auto* zset = obj->ZSet();
    return zset->Rank(args->element);
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::zsets
