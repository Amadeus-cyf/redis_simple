#include "server/commands/t_string/get.h"

#include <optional>

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_string {
void GetCommand::Exec(Client* const client) const {
  RS_LOG_DEBUG("get command called\n");
  StrArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (auto db = client->DB().lock()) {
    const auto opt_val = Get(db, &args);
    if (opt_val.has_value()) {
      client->AddReply(reply::FromBulkString(opt_val.value()));
    } else {
      client->AddReply(reply::Null());
    }
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int GetCommand::ParseArgs(const std::vector<std::string>& args,
                          StrArgs* str_args) const {
  if (args.empty()) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  str_args->key = args[0];
  return 0;
}

const std::optional<std::string> GetCommand::Get(
    std::shared_ptr<const db::RedisDb> db, const StrArgs* args) const {
  if (!db || !args) {
    return std::nullopt;
  }
  const auto* obj = db->LookupKey(args->key);
  if (obj && obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingString) {
    return std::nullopt;
  } else if (obj) {
    return std::optional<std::string>(obj->String());
  }
  return std::nullopt;
}
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
