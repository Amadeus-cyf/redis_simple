#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::sets {
namespace {
struct SIsMemberArgs {
  std::string_view key;
  std::string_view element;
};
int ParseArgs(const CommandArgs& args, SIsMemberArgs* sismember_args);
std::optional<int64_t> SIsMember(db::RedisDb* redis_db,
                                 const SIsMemberArgs* args);
}  // namespace

void HandleSIsMember(Client* const client) {
  SIsMemberArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = SIsMember(redis_db, &args);
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const CommandArgs& args, SIsMemberArgs* const sismember_args) {
  if (args.size() != 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  sismember_args->key = args[0];
  sismember_args->element = args[1];
  return 0;
}

std::optional<int64_t> SIsMember(db::RedisDb* redis_db,
                                 const SIsMemberArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kSet) {
    return std::nullopt;
  }
  try {
    const auto* set = obj->Set();
    return set->HasMember(args->element) ? 1 : 0;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::sets
