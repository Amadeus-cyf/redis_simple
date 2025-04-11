#include "server/commands/t_set/srem.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_set {
void SRemCommand::Exec(Client* const client) const {
  SRemArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    int r = SRem(db, &args);
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

int SRemCommand::ParseArgs(const std::vector<std::string>& args,
                           SRemArgs* const srem_args) const {
  if (args.size() < 2) {
    printf("invalid number of args\n");
    return -1;
  }
  srem_args->key = args[0];
  for (int i = 1; i < args.size(); ++i) {
    srem_args->elements.push_back(std::move(args[i]));
  }
  return 0;
}

int SRemCommand::SRem(std::shared_ptr<const db::RedisDb> db,
                      const SRemArgs* args) const {
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (!obj || obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingSet) {
    return -1;
  }
  try {
    set::Set* const set = obj->Set();
    int deleted = 0;
    for (const std::string& element : args->elements) {
      deleted += set->Remove(element) ? 1 : 0;
    }
    return deleted;
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
