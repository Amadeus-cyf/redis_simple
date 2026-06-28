#include <string>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::sets {
namespace {
struct SIsMemberArgs {
  std::string key;
  std::string element;
};
int ParseArgs(const std::vector<std::string>& args,
              SIsMemberArgs* sismember_args);
int SIsMember(db::RedisDb* redis_db, const SIsMemberArgs* args);
}  // namespace

void HandleSIsMember(Client* const client) {
  SIsMemberArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    int result = SIsMember(redis_db, &args);
    if (result < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(result));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args,
              SIsMemberArgs* const sismember_args) {
  if (args.size() != 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  sismember_args->key = args[0];
  sismember_args->element = args[1];
  return 0;
}

int SIsMember(db::RedisDb* redis_db, const SIsMemberArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kSet) {
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
}  // namespace
}  // namespace redis_simple::command::sets
