#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::command::zsets {
namespace {
struct ZRankArgs {
  std::string_view key;
  std::string_view element;
};
enum class ZRankStatus : std::uint8_t {
  kOk,
  kMissing,
  kWrongType,
};
struct ZRankResult {
  std::optional<size_t> rank;
  ZRankStatus status{ZRankStatus::kMissing};
};
int ParseArgs(const CommandArgs& args, ZRankArgs* zset_args);
ZRankResult ZRank(db::RedisDb* redis_db, const ZRankArgs* args);
}  // namespace

void HandleZRank(Client* const client) {
  ZRankArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = ZRank(redis_db, &args);
    if (result.status == ZRankStatus::kWrongType) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    if (result.rank.has_value()) {
      if (*result.rank >
          static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
        client->AddReply(reply::FromError("ERR rank out of range"));
        return;
      }
      client->AddReply(reply::FromInt64(static_cast<int64_t>(*result.rank)));
    } else {
      client->AddReply(reply::Null(client->Protocol()));
    }
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const CommandArgs& args, ZRankArgs* const zset_args) {
  if (args.size() != 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zset_args->key = args[0];
  zset_args->element = args[1];
  return 0;
}

ZRankResult ZRank(db::RedisDb* redis_db, const ZRankArgs* args) {
  if (redis_db == nullptr || args == nullptr) {
    return {std::nullopt, ZRankStatus::kMissing};
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    RS_LOG_DEBUG("key not found\n");
    return {std::nullopt, ZRankStatus::kMissing};
  }
  if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return {std::nullopt, ZRankStatus::kWrongType};
  }
  try {
    const auto* zset = obj->ZSet();
    return {zset->Rank(args->element), ZRankStatus::kOk};
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return {std::nullopt, ZRankStatus::kMissing};
  }
}
}  // namespace
}  // namespace redis_simple::command::zsets
