#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::command::sets {
namespace {
struct SMembersArgs {
  std::string_view key;
};
int ParseArgs(const CommandArgs& args, SMembersArgs* smembers_args);
std::optional<std::string> SMembers(db::RedisDb* redis_db,
                                    const SMembersArgs* args);
}  // namespace

void HandleSMembers(Client* const client) {
  SMembersArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    auto encoded = SMembers(redis_db, &args);
    if (!encoded.has_value()) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    client->AddReply(std::move(*encoded));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const CommandArgs& args, SMembersArgs* const smembers_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  smembers_args->key = args[0];
  return 0;
}

std::optional<std::string> SMembers(db::RedisDb* redis_db,
                                    const SMembersArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return reply::FromArrayHeader(0);
  }
  if (obj->Type() != db::RedisObject::ObjectType::kSet) {
    return std::nullopt;
  }
  const auto* set = obj->Set();
  std::string encoded = reply::FromArrayHeader(set->Size());
  set->ForEachMember([&encoded](std::string_view member) {
    reply::AppendBulkString(member, &encoded);
    return true;
  });
  return encoded;
}
}  // namespace
}  // namespace redis_simple::command::sets
