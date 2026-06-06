#include "server/commands/t_set/scard.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_set {
void SCardCommand::Exec(Client* const client) const {
  SAddArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    ssize_t r = SCard(db, &args);
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

int SCardCommand::ParseArgs(const std::vector<std::string>& args,
                            SAddArgs* const sadd_args) const {
  if (args.size() < 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  sadd_args->key = args[0];
  return 0;
}

ssize_t SCardCommand::SCard(std::shared_ptr<const db::RedisDb> db,
                            const SAddArgs* args) const {
  const auto* obj = db->LookupKey(args->key);
  if (!obj || obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  try {
    const auto* set = obj->Set();
    return set->Size();
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
