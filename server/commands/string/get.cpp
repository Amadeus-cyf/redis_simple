#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/commands/string/args.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::command::strings {
namespace {
enum class GetStatus : std::uint8_t {
  kOk,
  kMissing,
  kWrongType,
};

struct GetResult {
  const std::string* value;
  GetStatus status;
};

int ParseArgs(const CommandArgs& args, StringArgs* string_args);
GetResult Get(db::RedisDb* redis_db, const StringArgs* args);
}  // namespace

void HandleGet(Client* const client) {
  RS_LOG_DEBUG("get command called\n");
  StringArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto value_result = Get(redis_db, &args);
    if (value_result.status == GetStatus::kWrongType) {
      client->AddReply(reply::WrongTypeError());
    } else if (value_result.value != nullptr) {
      client->AddReply(reply::FromBulkString(*value_result.value));
    } else {
      client->AddReply(reply::Null(client->Protocol()));
    }
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const CommandArgs& args, StringArgs* string_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  string_args->key = args[0];
  string_args->value = {};
  string_args->expire = 0;
  string_args->flags = 0;
  return 0;
}

GetResult Get(db::RedisDb* redis_db, const StringArgs* args) {
  if (redis_db == nullptr || args == nullptr) {
    return {nullptr, GetStatus::kMissing};
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if ((obj != nullptr) && obj->Type() != db::RedisObject::ObjectType::kString) {
    return {nullptr, GetStatus::kWrongType};
  }
  if (obj != nullptr) {
    return {&obj->String(), GetStatus::kOk};
  }
  return {nullptr, GetStatus::kMissing};
}
}  // namespace
}  // namespace redis_simple::command::strings
