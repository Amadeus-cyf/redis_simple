#include "server/commands/t_set/srem.h"

#include <string>
#include <vector>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_set {
namespace {
struct SRemArgs {
  std::string key;
  std::vector<std::string> elements;
};
int ParseArgs(const std::vector<std::string>& args, SRemArgs* srem_args);
int SRem(const std::shared_ptr<db::RedisDb>& redis_db, const SRemArgs* args);
}  // namespace

void ExecuteSRem(Client* const client) {
  SRemArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto redis_db = client->DB().lock()) {
    int result = SRem(redis_db, &args);
    if (result < 0) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromInt64(result));
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
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
  for (int i = 1; i < args.size(); ++i) {
    srem_args->elements.push_back(args[i]);
  }
  return 0;
}

int SRem(const std::shared_ptr<db::RedisDb>& redis_db, const SRemArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  try {
    auto* const set = obj->Set();
    int deleted = 0;
    for (const auto& element : args->elements) {
      deleted += set->Remove(element) ? 1 : 0;
    }
    return deleted;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace
}  // namespace redis_simple::command::t_set
