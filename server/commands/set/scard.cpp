#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::sets {
namespace {
struct SCardArgs {
  std::string key;
};
int ParseArgs(const std::vector<std::string>& args, SCardArgs* scard_args);
std::optional<int64_t> ToReplyInteger(size_t value);
std::optional<int64_t> SCard(db::RedisDb* redis_db, const SCardArgs* args);
}  // namespace

void HandleSCard(Client* const client) {
  SCardArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = SCard(redis_db, &args);
    client->AddReply(result.has_value()
                         ? reply::FromInt64(*result)
                         : reply::FromInt64(reply::ReplyStatus::kError));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args,
              SCardArgs* const scard_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  scard_args->key = args[0];
  return 0;
}

std::optional<int64_t> ToReplyInteger(size_t value) {
  if (value > static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
    return std::nullopt;
  }
  return static_cast<int64_t>(value);
}

std::optional<int64_t> SCard(db::RedisDb* redis_db, const SCardArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kSet) {
    return std::nullopt;
  }
  try {
    const auto* set = obj->Set();
    return ToReplyInteger(set->Size());
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::sets
