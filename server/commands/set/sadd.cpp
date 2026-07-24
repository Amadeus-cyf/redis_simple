#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "data_types/set/set.h"
#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::command::sets {
namespace {
using Set = ::redis_simple::set::Set;

std::optional<int64_t> SAdd(db::RedisDb* redis_db, const CommandArgs& args);
}  // namespace

void HandleSAdd(Client* const client) {
  const auto& args = client->Args();
  if (args.size() < 2) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = SAdd(redis_db, args);
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

std::optional<int64_t> SAdd(db::RedisDb* redis_db, const CommandArgs& args) {
  if (redis_db == nullptr) {
    return std::nullopt;
  }
  auto* obj = redis_db->MutableLookupKey(args[0]);
  if ((obj != nullptr) && obj->Type() != db::RedisObject::ObjectType::kSet) {
    return std::nullopt;
  }
  if (obj == nullptr) {
    // LLVM 18 cannot trace unique_ptr ownership through Dict::Set.
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    auto new_obj = db::RedisObject::CreateWithSet(Set::Create());
    obj = new_obj.get();
    const auto status = redis_db->SetKey(args[0], std::move(new_obj), 0);
    if (status == db::DbStatus::kError) {
      return std::nullopt;
    }
  }
  try {
    auto* const set = obj->Set();
    int64_t added = 0;
    for (size_t i = 1; i < args.size(); ++i) {
      added += set->Add(args[i]) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::sets
