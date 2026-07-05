#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/commands/string/args.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

namespace redis_simple::command::strings {
namespace {
int ParseArgs(const std::vector<std::string>& args, StringArgs* string_args);
int ParseSetOption(const std::vector<std::string>& args, size_t* idx,
                   StringArgs* string_args);
bool ExpireAtFromTtl(int64_t ttl, int64_t multiplier, int64_t now,
                     int64_t* expire);
int Set(db::RedisDb* redis_db, const StringArgs* args);
}  // namespace

void HandleSet(Client* const client) {
  RS_LOG_DEBUG("set command called\n");
  StringArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::SyntaxError());
    return;
  }

  if (auto* redis_db = client->Db()) {
    if (Set(redis_db, &args) < 0) {
      client->AddReply(reply::FromError("ERR failed to set key"));
      return;
    }
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kOk));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args, StringArgs* string_args) {
  if (args.size() < 2) {
    RS_LOG_DEBUG("invalid args\n");
    return -1;
  }
  string_args->key = args[0];
  string_args->value = args[1];
  string_args->expire = 0;
  string_args->flags = 0;
  if (args.size() == 3) {
    int64_t now = utils::NowInMilliseconds();
    int64_t ttl = 0;
    if (utils::ToInt64(args[2], &ttl) &&
        ExpireAtFromTtl(ttl, 1, now, &string_args->expire)) {
      return 0;
    }
  }
  for (size_t i = 2; i < args.size();) {
    if (ParseSetOption(args, &i, string_args) < 0) {
      return -1;
    }
  }
  return 0;
}

int ParseSetOption(const std::vector<std::string>& args, size_t* const idx,
                   StringArgs* const string_args) {
  std::string option = args[*idx];
  utils::ToUppercase(option);
  if (option == "KEEPTTL") {
    if (string_args->expire > 0) {
      return -1;
    }
    string_args->flags |= db::ToInt(db::SetKeyFlag::kKeepTtl);
    ++(*idx);
    return 0;
  }
  if (option != "EX" && option != "PX") {
    return -1;
  }
  if ((string_args->flags & db::ToInt(db::SetKeyFlag::kKeepTtl)) != 0 ||
      string_args->expire > 0 || *idx + 1 >= args.size()) {
    return -1;
  }
  int64_t ttl = 0;
  if (!utils::ToInt64(args[*idx + 1], &ttl) || ttl <= 0) {
    return -1;
  }
  constexpr int64_t kMillisecondsPerSecond = 1000;
  const int64_t multiplier = option == "EX" ? kMillisecondsPerSecond : 1;
  if (!ExpireAtFromTtl(ttl, multiplier, utils::NowInMilliseconds(),
                       &string_args->expire)) {
    return -1;
  }
  *idx += 2;
  return 0;
}

bool ExpireAtFromTtl(int64_t ttl, int64_t multiplier, int64_t now,
                     int64_t* expire) {
  if (ttl <= 0) {
    return false;
  }
  if (ttl > std::numeric_limits<int64_t>::max() / multiplier) {
    return false;
  }
  const int64_t ttl_ms = ttl * multiplier;
  if (ttl_ms > std::numeric_limits<int64_t>::max() - now) {
    return false;
  }
  *expire = now + ttl_ms;
  return true;
}

int Set(db::RedisDb* redis_db, const StringArgs* args) {
  auto value = db::RedisObject::CreateWithString(args->value);
  const auto status =
      redis_db->SetKey(args->key, std::move(value), args->expire, args->flags);
  return status == db::DbStatus::kError ? -1 : 0;
}
}  // namespace
}  // namespace redis_simple::command::strings
