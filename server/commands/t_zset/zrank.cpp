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
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    const std::optional<size_t>& opt_rank = ZRank(db, &args);
    if (opt_rank.has_value()) {
      client->AddReply(reply::FromInt64(opt_rank.value()));
    } else {
      client->AddReply(reply::Null());
    }
  } else {
    printf("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int ZRankCommand::ParseArgs(const std::vector<std::string>& args,
                            ZRankArgs* const zset_args) const {
  if (args.size() < 2) {
    printf("invalid number of args\n");
    return -1;
  }
  zset_args->key = std::move(args[0]);
  zset_args->ele = std::move(args[1]);
  return 0;
}

const std::optional<size_t> ZRankCommand::ZRank(
    std::shared_ptr<const db::RedisDb> db, const ZRankArgs* args) const {
  if (!db || !args) {
    return std::nullopt;
  }
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (!obj) {
    printf("key not found\n");
    return std::nullopt;
  }
  if (obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return std::nullopt;
  }
  try {
    const zset::ZSet* zset = obj->ZSet();
    return zset->GetRankOfKey(args->ele);
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
