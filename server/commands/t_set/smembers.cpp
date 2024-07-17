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
    client->AddReply(reply::FromInt64(reply::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    std::vector<std::string> members;
    if (SMembers(db, &args, members) < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    auto to_string = [](const std::string& member) { return member; };
    const std::optional<const std::string>& opt =
        reply_utils::EncodeList<std::string, to_string>(members);
    if (!opt.has_value()) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->AddReply(opt.value());
  } else {
    printf("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int SMembersCommand::ParseArgs(const std::vector<std::string>& args,
                               SMembersArgs* const smembers_args) const {
  if (args.size() < 1) {
    printf("invalid number of args\n");
    return -1;
  }
  smembers_args->key = args[0];
  return 0;
}

int SMembersCommand::SMembers(std::shared_ptr<const db::RedisDb> db,
                              const SMembersArgs* args,
                              std::vector<std::string>& members) const {
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (!obj || obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingSet) {
    return -1;
  }
  const set::Set* set = obj->Set();
  const std::vector<std::string>& set_members = set->ListAllMembers();
  members.insert(members.begin(), set_members.begin(), set_members.end());
  return 0;
}
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
