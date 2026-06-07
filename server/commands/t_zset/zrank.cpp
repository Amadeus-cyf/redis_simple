#include "server/commands/t_zset/zrank.h"

#include <optional>

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZRankCommand::Exec(Client* const client) const {
  ZRankArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    const auto opt_rank = ZRank(db, &args);
    if (opt_rank.has_value()) {
      client->AddReply(reply::FromInt64(*opt_rank));
    } else {
      client->AddReply(reply::Null());
    }
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

int ZRankCommand::ParseArgs(const std::vector<std::string>& args,
                            ZRankArgs* const zset_args) const {
  if (args.size() < 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zset_args->key = args[0];
  zset_args->ele = args[1];
  return 0;
}

const std::optional<size_t> ZRankCommand::ZRank(
    std::shared_ptr<const db::RedisDb> db, const ZRankArgs* args) const {
  if (!db || !args) {
    return std::nullopt;
  }
  const auto* obj = db->LookupKey(args->key);
  if (!obj) {
    RS_LOG_DEBUG("key not found\n");
    return std::nullopt;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return std::nullopt;
  }
  try {
    const auto* zset = obj->ZSet();
    return zset->GetRankOfKey(args->ele);
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
