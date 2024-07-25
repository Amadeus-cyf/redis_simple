#include "server/commands/t_zset/zscore.h"

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZScoreCommand::Exec(Client* const client) const {
  ZScoreArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    const std::optional<double>& opt_score = ZScore(db, &args);
    if (opt_score.has_value()) {
      client->AddReply(reply::FromFloat(opt_score.value()));
    } else {
      client->AddReply(reply::Null());
    }
  } else {
    printf("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int ZScoreCommand::ParseArgs(const std::vector<std::string>& args,
                             ZScoreArgs* const zscore_args) const {
  if (args.size() < 2) {
    printf("invalid number of args\n");
    return -1;
  }
  zscore_args->key = args[0];
  zscore_args->element = args[1];
  return 0;
}

const std::optional<double> ZScoreCommand::ZScore(
    std::shared_ptr<const db::RedisDb> db, const ZScoreArgs* args) const {
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (!obj || obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    return std::nullopt;
  }
  try {
    const zset::ZSet* zset = obj->ZSet();
    return zset->GetScoreOfKey(args->element);
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
