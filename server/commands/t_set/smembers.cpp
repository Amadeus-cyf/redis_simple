#include "server/commands/t_set/smembers.h"

#include "server/client.h"
#include "server/reply/reply.h"
#include "server/reply_utils/reply_utils.h"

namespace redis_simple {
namespace command {
namespace t_set {
void SMembersCommand::Exec(Client* const client) const {
  SMembersArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    std::vector<std::string> members;
    if (SMembers(db, &args, members) < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    auto to_string = [](const std::string& member) { return member; };
    const auto result =
        reply_utils::EncodeList<std::string, to_string>(members);
    if (!result.has_value()) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(*result);
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

int SMembersCommand::ParseArgs(const std::vector<std::string>& args,
                               SMembersArgs* const smembers_args) const {
  if (args.size() < 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  smembers_args->key = args[0];
  return 0;
}

int SMembersCommand::SMembers(std::shared_ptr<const db::RedisDb> db,
                              const SMembersArgs* args,
                              std::vector<std::string>& members) const {
  const auto* obj = db->LookupKey(args->key);
  if (!obj || obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  const auto* set = obj->Set();
  members = set->ListAllMembers();
  return 0;
}
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
