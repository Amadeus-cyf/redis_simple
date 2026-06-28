#include "server/commands/t_set/smembers.h"

#include <string>
#include <vector>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "server/reply_utils/reply_utils.h"

namespace redis_simple::command::t_set {
namespace {
struct SMembersArgs {
  std::string key;
};
int ParseArgs(const std::vector<std::string>& args,
              SMembersArgs* smembers_args);
int SMembers(db::RedisDb* redis_db, const SMembersArgs* args,
             std::vector<std::string>& members);
}  // namespace

void ExecuteSMembers(Client* const client) {
  SMembersArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    std::vector<std::string> members;
    if (SMembers(redis_db, &args, members) < 0) {
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
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
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
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  const auto* set = obj->Set();
  members = set->ListAllMembers();
  return 0;
}
}  // namespace
}  // namespace redis_simple::command::t_set
