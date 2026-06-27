#include "server/commands/t_set/scard.h"

#include <limits>

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_set {
void SCardCommand::Exec(Client* const client) const {
  SCardArgs args;
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
                            SCardArgs* const scard_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  scard_args->key = args[0];
  return 0;
}

ssize_t SCardCommand::SCard(const std::shared_ptr<db::RedisDb>& db,
                            const SCardArgs* args) {
  const auto* obj = db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  try {
    const auto* set = obj->Set();
    const size_t size = set->Size();
    if (size > static_cast<size_t>(std::numeric_limits<ssize_t>::max())) {
      return -1;
    }
    return static_cast<ssize_t>(size);
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace redis_simple::command::t_set
