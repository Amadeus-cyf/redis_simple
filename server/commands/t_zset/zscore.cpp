#include "server/commands/t_zset/zscore.h"

#include <optional>
#include <string>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_zset {
namespace {
struct ZScoreArgs {
  std::string key;
  std::string element;
};
int ParseArgs(const std::vector<std::string>& args, ZScoreArgs* zscore_args);
std::optional<double> ZScore(db::RedisDb* redis_db, const ZScoreArgs* args);
}  // namespace

void ExecuteZScore(Client* const client) {
  ZScoreArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto opt_score = ZScore(redis_db, &args);
    if (opt_score.has_value()) {
      client->AddReply(reply::FromFloat(*opt_score));
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
              ZScoreArgs* const zscore_args) {
  if (args.size() != 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zscore_args->key = args[0];
  zscore_args->element = args[1];
  return 0;
}

std::optional<double> ZScore(db::RedisDb* redis_db, const ZScoreArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if ((obj == nullptr) ||
      obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
    return std::nullopt;
  }
  try {
    const auto* zset = obj->ZSet();
    return zset->GetScoreOfKey(args->element);
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::t_zset
