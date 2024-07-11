#include "server/commands/t_set/sadd.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_set {
void SAddCommand::Exec(Client* const client) const {
  SAddArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    int r = SAdd(db, &args);
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

int SAddCommand::ParseArgs(const std::vector<std::string>& args,
                           SAddArgs* const sadd_args) const {
  if (args.size() < 2) {
    printf("invalid number args\n");
    return -1;
  }
  sadd_args->key = args[0];
  for (int i = 1; i < args.size(); ++i) {
    sadd_args->elements.push_back(args[i]);
  }
  return 0;
}

int SAddCommand::SAdd(std::shared_ptr<const db::RedisDb> db,
                      const SAddArgs* args) const {
  if (!db || !args) {
    return -1;
  }
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (obj && obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingSet) {
    return -1;
  }
  if (!obj) {
    obj = db::RedisObj::CreateWithSet(set::Set::Init());
    int r = db->SetKey(args->key, obj, 0);
    obj->DecrRefCount();
    if (r < 0) {
      return -1;
    }
  }
  try {
    set::Set* const set = obj->Set();
    int added = 0;
    for (const std::string& element : args->elements) {
      added += set->Add(element) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
