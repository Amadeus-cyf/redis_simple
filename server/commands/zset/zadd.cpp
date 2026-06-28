#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "storage/zset/zset.h"

namespace redis_simple::command::zsets {
namespace {
using ZSet = ::redis_simple::zset::ZSet;

struct ZAddArgs {
  std::string key;
  std::vector<std::pair<std::string, double>> element_scores;
};
int ParseArgs(const std::vector<std::string>& args, ZAddArgs* zset_args);
int ZAdd(db::RedisDb* redis_db, const ZAddArgs* args);
}  // namespace

void HandleZAdd(Client* const client) {
  ZAddArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    int result = ZAdd(redis_db, &args);
    if (result < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(result));
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int ParseArgs(const std::vector<std::string>& args, ZAddArgs* const zset_args) {
  if (args.size() < 3 || args.size() % 2 == 0) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zset_args->key = args[0];
  for (int i = 1; i < args.size() - 1; i += 2) {
    const std::string& element = args[i + 1];
    double score = 0.0;
    try {
      score = stod(args[i]);
    } catch (const std::exception&) {
      RS_LOG_DEBUG("invalid args format\n");
      return -1;
    }
    zset_args->element_scores.emplace_back(element, score);
  }
  return 0;
}

int ZAdd(db::RedisDb* redis_db, const ZAddArgs* args) {
  if (redis_db == nullptr || args == nullptr) {
    return -1;
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if ((obj != nullptr) && obj->Type() != db::RedisObject::ObjectType::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return -1;
  }
  if (obj == nullptr) {
    auto new_obj =
        db::RedisObject::CreateWithZSet(std::unique_ptr<ZSet>(ZSet::Init()));
    obj = new_obj.get();
    const auto status = redis_db->SetKey(args->key, std::move(new_obj), 0);
    if (status == db::DbStatus::kError) {
      return -1;
    }
  }
  try {
    auto* zset = obj->ZSet();
    int added = 0;
    for (const auto& element_score : args->element_scores) {
      const std::string& element = element_score.first;
      const double score = element_score.second;
      added += zset->InsertOrUpdate(element, score) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace
}  // namespace redis_simple::command::zsets
