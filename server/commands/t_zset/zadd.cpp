#include "server/commands/t_zset/zadd.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_zset {
void ZAddCommand::Exec(Client* const client) const {
  ZAddArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    int r = ZAdd(db, &args);
    if (r < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    client->AddReply(reply::FromInt64(r));
  } else {
    printf("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int ZAddCommand::ParseArgs(const std::vector<std::string>& args,
                           ZAddArgs* const zset_args) const {
  if (args.size() < 3) {
    printf("invalid number of args\n");
    return -1;
  }
  const std::string& key = args[0];
  zset_args->key = key;
  for (int i = 1; i < args.size() - 1; i += 2) {
    const std::string& ele = args[i + 1];
    double score = 0.0;
    try {
      score = stod(args[i]);
    } catch (const std::exception&) {
      printf("invalid args format\n");
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
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (obj && obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return -1;
  }
  if (!obj) {
    obj = db::RedisObj::CreateWithZSet(zset::ZSet::Init());
    int r = db->SetKey(args->key, obj, 0) == db::DBStatus::dbErr;
    obj->DecrRefCount();
    if (r < 0) {
      return -1;
    }
  }
  try {
    zset::ZSet* const zset = obj->ZSet();
    int added = 0;
    for (std::pair<std::string, double> ele_score : args->ele_score_list) {
      const std::string& element = ele_score.first;
      const double score = ele_score.second;
      added += zset->InsertOrUpdate(element, score) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    printf("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
