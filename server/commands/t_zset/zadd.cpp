#include "server/commands/t_zset/zadd.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZAddCommand::Exec(Client* const client) const {
  ZAddArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  if (auto db = client->DB().lock()) {
    int r = ZAdd(db, &args);
    if (r < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(r));
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

int ZAddCommand::ParseArgs(const std::vector<std::string>& args,
                           ZAddArgs* const zset_args) const {
  if (args.size() < 3) {
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
    zset_args->ele_score_list.push_back({ele, score});
  }
  return 0;
}

int ZAddCommand::ZAdd(std::shared_ptr<const db::RedisDb> db,
                      const ZAddArgs* args) const {
  if (!db || !args) {
    return -1;
  }
  const auto* obj = db->LookupKey(args->key);
  if (obj && obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return -1;
  }
  if (!obj) {
    obj = db::RedisObject::CreateWithZSet(zset::ZSet::Init());
    int r = db->SetKey(args->key, obj, 0) == db::DbStatus::kError;
    obj->DecrRefCount();
    if (r < 0) {
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
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
