#include "server/commands/t_zset/zadd.h"

#include <string>
#include <utility>
#include <vector>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "storage/zset/zset.h"

namespace redis_simple::command::t_zset {
namespace {
struct ZAddArgs {
  std::string key;
  std::vector<std::pair<std::string, double>> ele_score_list;
};
int ParseArgs(const std::vector<std::string>& args, ZAddArgs* zset_args);
int ZAdd(db::RedisDb* redis_db, const ZAddArgs* args);
}  // namespace

void ExecuteZAdd(Client* const client) {
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
    const std::string& ele = args[i + 1];
    double score = 0.0;
    try {
      score = stod(args[i]);
    } catch (const std::exception&) {
      RS_LOG_DEBUG("invalid args format\n");
      return -1;
    }
    zset_args->ele_score_list.emplace_back(ele, score);
  }
  return 0;
}

int ZAdd(db::RedisDb* redis_db, const ZAddArgs* args) {
  if (!redis_db || (args == nullptr)) {
    return -1;
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if ((obj != nullptr) &&
      obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return -1;
  }
  if (obj == nullptr) {
    obj = db::RedisObject::CreateWithZSet(zset::ZSet::Init());
    const auto status = redis_db->SetKey(args->key, obj, 0);
    obj->DecrRefCount();
    if (status == db::DbStatus::kError) {
      return -1;
    }
  }
  try {
    auto* zset = obj->ZSet();
    int added = 0;
    for (const auto& ele_score : args->ele_score_list) {
      const std::string& element = ele_score.first;
      const double score = ele_score.second;
      added += zset->InsertOrUpdate(element, score) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace
}  // namespace redis_simple::command::t_zset
