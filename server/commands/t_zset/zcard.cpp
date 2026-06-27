#include "server/commands/t_zset/zcard.h"

#include <limits>

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_zset {
void ZCardCommand::Exec(Client* const client) const {
  ZCardArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    ssize_t r = ZCard(db, &args);
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

int ZCardCommand::ParseArgs(const std::vector<std::string>& args,
                            ZCardArgs* const zcard_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zcard_args->key = args[0];
  return 0;
}

ssize_t ZCardCommand::ZCard(const std::shared_ptr<db::RedisDb>& db,
                            const ZCardArgs* args) {
  const auto* obj = db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
    return -1;
  }
  try {
    const auto* zset = obj->ZSet();
    const size_t size = zset->Size();
    if (size > static_cast<size_t>(std::numeric_limits<ssize_t>::max())) {
      return -1;
    }
    return static_cast<ssize_t>(size);
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace redis_simple::command::t_zset
