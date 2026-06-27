#include "server/commands/t_set/sismember.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_set {
void SIsMemberCommand::Exec(Client* const client) const {
  SIsMemberArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    int r = SIsMember(db, &args);
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

int SIsMemberCommand::ParseArgs(const std::vector<std::string>& args,
                                SIsMemberArgs* const sismember_args) {
  if (args.size() != 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  sismember_args->key = args[0];
  sismember_args->element = args[1];
  return 0;
}

int SIsMemberCommand::SIsMember(const std::shared_ptr<db::RedisDb>& db,
                                const SIsMemberArgs* args) {
  const auto* obj = db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  try {
    const auto* set = obj->Set();
    return set->HasMember(args->element) ? 1 : 0;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace redis_simple::command::t_set
