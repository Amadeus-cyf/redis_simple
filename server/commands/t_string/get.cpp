#include "server/commands/t_string/get.h"

#include <optional>

#include "server/client.h"
#include "server/commands/t_string/args.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_string {
namespace {
int ParseArgs(const std::vector<std::string>& args, StringArgs* string_args);
std::optional<std::string> Get(const std::shared_ptr<db::RedisDb>& redis_db,
                               const StringArgs* args);
}  // namespace

void ExecuteGet(Client* const client) {
  RS_LOG_DEBUG("get command called\n");
  StringArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto redis_db = client->DB().lock()) {
    const auto value_result = Get(redis_db, &args);
    if (value_result.has_value()) {
      client->AddReply(reply::FromBulkString(*value_result));
    } else {
      client->AddReply(reply::Null());
    }
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args, StringArgs* string_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  string_args->key = args[0];
  return 0;
}

std::optional<std::string> Get(const std::shared_ptr<db::RedisDb>& redis_db,
                               const StringArgs* args) {
  if (!redis_db || (args == nullptr)) {
    return std::nullopt;
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if ((obj != nullptr) &&
      obj->Encoding() != db::RedisObject::ObjEncoding::kString) {
    return std::nullopt;
  }
  if (obj) {
    return obj->String();
  }
  return std::nullopt;
}
}  // namespace
}  // namespace redis_simple::command::t_string
