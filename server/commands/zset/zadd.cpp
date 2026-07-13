#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "data_types/zset/zset.h"
#include "logging/logger.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "utils/float_utils.h"

namespace redis_simple::command::zsets {
namespace {
using ZSet = ::redis_simple::zset::ZSet;

struct ZAddArgs {
  std::string_view key;
  std::vector<std::pair<std::string_view, double>> element_scores;
};
int ParseArgs(const CommandArgs& args, ZAddArgs* zset_args);
std::optional<int64_t> ZAdd(db::RedisDb* redis_db, const ZAddArgs* args);
}  // namespace

void HandleZAdd(Client* const client) {
  ZAddArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::SyntaxError());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = ZAdd(redis_db, &args);
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromError("ERR db unavailable"));
  }
}

namespace {

int ParseArgs(const CommandArgs& args, ZAddArgs* const zset_args) {
  if (args.size() < 3 || args.size() % 2 == 0) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zset_args->key = args[0];
  for (size_t i = 1; i < args.size() - 1; i += 2) {
    std::string_view element = args[i + 1];
    double score = 0.0;
    if (!utils::ToDouble(args[i], &score)) {
      RS_LOG_DEBUG("invalid args format\n");
      return -1;
    }
    zset_args->element_scores.emplace_back(element, score);
  }
  return 0;
}

std::optional<int64_t> ZAdd(db::RedisDb* redis_db, const ZAddArgs* args) {
  if (redis_db == nullptr || args == nullptr) {
    return std::nullopt;
  }
  auto* obj = redis_db->MutableLookupKey(args->key);
  if ((obj != nullptr) && obj->Type() != db::RedisObject::ObjectType::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return std::nullopt;
  }
  if (obj == nullptr) {
    auto new_obj = db::RedisObject::CreateWithZSet(ZSet::Create());
    obj = new_obj.get();
    const auto status = redis_db->SetKey(args->key, std::move(new_obj), 0);
    if (status == db::DbStatus::kError) {
      return std::nullopt;
    }
  }
  try {
    auto* zset = obj->ZSet();
    int64_t added = 0;
    for (const auto& element_score : args->element_scores) {
      std::string_view element = element_score.first;
      const double score = element_score.second;
      added += zset->InsertOrUpdate(element, score) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return std::nullopt;
  }
}
}  // namespace
}  // namespace redis_simple::command::zsets
