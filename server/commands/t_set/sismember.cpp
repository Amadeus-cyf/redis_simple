#include "server/commands/t_set/sismember.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_set {
void SIsMemberCommand::Exec(Client* const client) const {
  SIsMemberArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    int r = SIsMember(db, &args);
    if (r < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->AddReply(reply::FromInt64(r));
  } else {
    printf("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int SIsMemberCommand::ParseArgs(const std::vector<std::string>& args,
                                SIsMemberArgs* const sismember_args) const {
  if (args.size() < 2) {
    printf("invalid number of args\n");
    return -1;
  }
  sismember_args->key = std::move(args[0]);
  sismember_args->element = std::move(args[1]);
  return 0;
}

int SIsMemberCommand::SIsMember(std::shared_ptr<const db::RedisDb> db,
                                const SIsMemberArgs* args) const {
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (!obj || obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingSet) {
    return -1;
  }
  try {
    const set::Set* set = obj->Set();
    return set->HasMember(args->element) ? 1 : 0;
  } catch (std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
