#include "server/commands/t_set/sadd.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_set {
void SAddCommand::Exec(Client* const client) const {
  SAddArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    int r = SAdd(db, &args);
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

int SAddCommand::ParseArgs(const std::vector<std::string>& args,
                           SAddArgs* const sadd_args) const {
  if (args.size() < 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  sadd_args->key = args[0];
  for (int i = 1; i < args.size(); ++i) {
    sadd_args->elements.push_back(args[i]);
  }
  return 0;
}

int SAddCommand::SAdd(std::shared_ptr<db::RedisDb> db,
                      const SAddArgs* args) const {
  if (!db || !args) {
    return -1;
  }
  const auto* obj = db->LookupKey(args->key);
  if (obj && obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  if (!obj) {
    obj = db::RedisObject::CreateWithSet(set::Set::Init());
    const auto status = db->SetKey(args->key, obj, 0);
    obj->DecrRefCount();
    if (status == db::DbStatus::kError) {
      return -1;
    }
  }
  try {
    auto* const set = obj->Set();
    int added = 0;
    for (const auto& element : args->elements) {
      added += set->Add(element) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_set
}  // namespace command
}  // namespace redis_simple
