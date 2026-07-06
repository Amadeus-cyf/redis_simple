#include <string>
#include <vector>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::sets {
namespace {
struct SMembersArgs {
  std::string key;
};
int ParseArgs(const std::vector<std::string>& args,
              SMembersArgs* smembers_args);
int SMembers(db::RedisDb* redis_db, const SMembersArgs* args,
             std::vector<std::string>& members);
}  // namespace

void HandleSMembers(Client* const client) {
  SMembersArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    std::vector<std::string> members;
    if (SMembers(redis_db, &args, members) < 0) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    client->AddReply(reply::FromBulkStringArray(members));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args,
              SMembersArgs* const smembers_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  smembers_args->key = args[0];
  return 0;
}

int SMembers(db::RedisDb* redis_db, const SMembersArgs* args,
             std::vector<std::string>& members) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kSet) {
    return -1;
  }
  const auto* set = obj->Set();
  members = set->ListAllMembers();
  return 0;
}
}  // namespace
}  // namespace redis_simple::command::sets
