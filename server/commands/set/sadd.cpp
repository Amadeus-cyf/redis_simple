#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "data_types/set/set.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::sets {
namespace {
using Set = ::redis_simple::set::Set;

struct SAddArgs {
  std::string key;
  std::vector<std::string> elements;
};
int ParseArgs(const std::vector<std::string>& args, SAddArgs* sadd_args);
std::optional<int64_t> SAdd(db::RedisDb* redis_db, const SAddArgs* args);
}  // namespace

void HandleSAdd(Client* const client) {
  SAddArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = SAdd(redis_db, &args);
    client->AddReply(result.has_value()
                         ? reply::FromInt64(*result)
                         : reply::FromInt64(reply::ReplyStatus::kError));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args, SAddArgs* const sadd_args) {
  if (args.size() < 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  sadd_args->key = args[0];
  for (size_t i = 1; i < args.size(); ++i) {
    sadd_args->elements.push_back(args[i]);
  }
  return 0;
}

std::optional<int64_t> SAdd(db::RedisDb* redis_db, const SAddArgs* args) {
  if (redis_db == nullptr || args == nullptr) {
    return std::nullopt;
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if ((obj != nullptr) && obj->Type() != db::RedisObject::ObjectType::kSet) {
    return std::nullopt;
  }
  if (obj == nullptr) {
    auto new_obj = db::RedisObject::CreateWithSet(Set::Create());
    obj = new_obj.get();
    const auto status = redis_db->SetKey(args->key, std::move(new_obj), 0);
    if (status == db::DbStatus::kError) {
      return std::nullopt;
    }
  }
  try {
    auto* const set = obj->Set();
    int64_t added = 0;
    for (const auto& element : args->elements) {
      added += set->Add(element) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::sets
