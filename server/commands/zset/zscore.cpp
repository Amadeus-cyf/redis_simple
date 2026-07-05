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
struct ZScoreArgs {
  std::string key;
  std::string element;
};
enum class ZScoreStatus : std::uint8_t {
  kOk,
  kMissing,
  kWrongType,
};
struct ZScoreResult {
  std::optional<double> score;
  ZScoreStatus status;
};
int ParseArgs(const std::vector<std::string>& args, ZScoreArgs* zscore_args);
ZScoreResult ZScore(db::RedisDb* redis_db, const ZScoreArgs* args);
}  // namespace

void HandleZScore(Client* const client) {
  ZScoreArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = ZScore(redis_db, &args);
    if (result.status == ZScoreStatus::kWrongType) {
      client->AddReply(reply::WrongTypeError());
    } else if (result.score.has_value()) {
      client->AddReply(reply::FromFloat(*result.score));
    } else {
      client->AddReply(reply::Null());
    }
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args,
              ZScoreArgs* const zscore_args) {
  if (args.size() != 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zscore_args->key = args[0];
  zscore_args->element = args[1];
  return 0;
}

ZScoreResult ZScore(db::RedisDb* redis_db, const ZScoreArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return {std::nullopt, ZScoreStatus::kMissing};
  }
  if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
    return {std::nullopt, ZScoreStatus::kWrongType};
  }
  try {
    const auto* zset = obj->ZSet();
    return {zset->Score(args->element), ZScoreStatus::kOk};
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return {std::nullopt, ZScoreStatus::kMissing};
  }
}
}  // namespace
}  // namespace redis_simple::command::zsets
