#include "server/commands/t_string/set.h"

#include "server/client.h"
#include "server/reply/reply.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

namespace redis_simple::command::t_string {
void SetCommand::Exec(Client* const client) const {
  RS_LOG_DEBUG("set command called\n");
  StringArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    if (Set(db, &args) < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kOk));
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

int SetCommand::ParseArgs(const std::vector<std::string>& args,
                          StringArgs* string_args) {
  if (args.size() < 2 || args.size() > 3) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  string_args->key = args[0];
  string_args->val = args[1];
  string_args->expire = 0;
  if (args.size() >= 3) {
    int64_t now = utils::GetNowInMilliseconds();
    int64_t ttl = 0;
    if (utils::ToInt64(args[2], &ttl)) {
      string_args->expire = now + ttl;
    } else {
      RS_LOG_DEBUG("invalid args\n");
      return -1;
    }
  }
  return 0;
}

int SetCommand::Set(const std::shared_ptr<db::RedisDb>& db,
                    const StringArgs* args) {
  const auto* val = db::RedisObject::CreateWithString(args->val);
  const auto status = db->SetKey(args->key, val, args->expire, 0);
  val->DecrRefCount();
  return status == db::DbStatus::kError ? -1 : 0;
}
}  // namespace redis_simple::command::t_string
