#include <string>
#include <string_view>
#include <vector>

#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/db/redis_obj.h"
#include "server/reply/reply.h"

namespace redis_simple::command::key {
namespace {
struct TypeArgs {
  std::string_view key;
};

int ParseTypeArgs(const CommandArgs& args, TypeArgs* type_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  type_args->key = args[0];
  return 0;
}

std::string_view ObjectTypeName(const db::RedisObject* const object) {
  if (object == nullptr) {
    return "none";
  }

  switch (object->Type()) {
    case db::RedisObject::ObjectType::kString:
      return "string";
    case db::RedisObject::ObjectType::kSet:
      return "set";
    case db::RedisObject::ObjectType::kList:
      return "list";
    case db::RedisObject::ObjectType::kZSet:
      return "zset";
    case db::RedisObject::ObjectType::kHash:
      return "hash";
  }
  return "none";
}
}  // namespace

void HandleType(Client* const client) {
  RS_LOG_DEBUG("type command called\n");
  TypeArgs args;
  if (ParseTypeArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    client->AddReply(
        reply::FromString(ObjectTypeName(redis_db->LookupKey(args.key))));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}
}  // namespace redis_simple::command::key
