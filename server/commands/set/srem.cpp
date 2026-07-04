#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::sets {
namespace {
struct SRemArgs {
  std::string key;
  std::vector<std::string> elements;
};
int ParseArgs(const std::vector<std::string>& args, SRemArgs* srem_args);
std::optional<int64_t> SRem(db::RedisDb* redis_db, const SRemArgs* args);
}  // namespace

void HandleSRem(Client* const client) {
  SRemArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = SRem(redis_db, &args);
    client->AddReply(result.has_value()
                         ? reply::FromInt64(*result)
                         : reply::FromInt64(reply::ReplyStatus::kError));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args, SRemArgs* const srem_args) {
  if (args.size() < 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  srem_args->key = args[0];
  for (size_t i = 1; i < args.size(); ++i) {
    srem_args->elements.push_back(args[i]);
  }
  return 0;
}

std::optional<int64_t> SRem(db::RedisDb* redis_db, const SRemArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Type() != db::RedisObject::ObjectType::kSet) {
    return std::nullopt;
  }
  try {
    auto* const set = obj->Set();
    int64_t deleted = 0;
    for (const auto& element : args->elements) {
      deleted += set->Remove(element) ? 1 : 0;
    }
    return deleted;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::sets
