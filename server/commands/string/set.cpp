#include <utility>

#include "server/client.h"
#include "server/commands/string/args.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

namespace redis_simple::command::strings {
namespace {
int ParseArgs(const std::vector<std::string>& args, StringArgs* string_args);
int Set(db::RedisDb* redis_db, const StringArgs* args);
}  // namespace

void HandleSet(Client* const client) {
  RS_LOG_DEBUG("set command called\n");
  StringArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    if (Set(redis_db, &args) < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kOk));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args, StringArgs* string_args) {
  if (args.size() < 2 || args.size() > 3) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  string_args->key = args[0];
  string_args->value = args[1];
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

int Set(db::RedisDb* redis_db, const StringArgs* args) {
  auto value = db::RedisObject::CreateWithString(args->value);
  const auto status =
      redis_db->SetKey(args->key, std::move(value), args->expire, 0);
  return status == db::DbStatus::kError ? -1 : 0;
}
}  // namespace
}  // namespace redis_simple::command::strings
