#include "server/commands/t_string/get.h"

#include <optional>

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_string {
void GetCommand::Exec(Client* const client) const {
  RS_LOG_DEBUG("get command called\n");
  StringArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    const auto value_result = Get(db, &args);
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

int GetCommand::ParseArgs(const std::vector<std::string>& args,
                          StringArgs* string_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  string_args->key = args[0];
  return 0;
}

std::optional<std::string> GetCommand::Get(
    const std::shared_ptr<db::RedisDb>& db, const StringArgs* args) {
  if (!db || (args == nullptr)) {
    return std::nullopt;
  }
  const auto* obj = db->LookupKey(args->key);
  if ((obj != nullptr) &&
      obj->Encoding() != db::RedisObject::ObjEncoding::kString) {
    return std::nullopt;
  }
  if (obj) {
    return obj->String();
  }
  return std::nullopt;
}
}  // namespace redis_simple::command::t_string
