#include "server/commands/t_set/sadd.h"

#include <string>
#include <vector>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "storage/set/set.h"

namespace redis_simple::command::t_set {
namespace {
struct SAddArgs {
  std::string key;
  std::vector<std::string> elements;
};
int ParseArgs(const std::vector<std::string>& args, SAddArgs* sadd_args);
int SAdd(const std::shared_ptr<db::RedisDb>& redis_db, const SAddArgs* args);
}  // namespace

void ExecuteSAdd(Client* const client) {
  SAddArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto redis_db = client->DB().lock()) {
    int result = SAdd(redis_db, &args);
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

int ParseArgs(const std::vector<std::string>& args, SAddArgs* const sadd_args) {
  if (args.size() < 2) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  sadd_args->key = args[0];
  for (int i = 1; i < args.size(); ++i) {
    sadd_args->elements.push_back(args[i]);
  }
  return 0;
}

int SAdd(const std::shared_ptr<db::RedisDb>& redis_db, const SAddArgs* args) {
  if (!redis_db || (args == nullptr)) {
    return -1;
  }
  const auto* obj = redis_db->LookupKey(args->key);
  if ((obj != nullptr) &&
      obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  if (obj == nullptr) {
    obj = db::RedisObject::CreateWithSet(set::Set::Init());
    const auto status = redis_db->SetKey(args->key, obj, 0);
    obj->DecrRefCount();
    if (status == db::DbStatus::kError) {
      return -1;
    }
  }
  try {
    auto* const set = obj->Set();
    int added = 0;
    for (const auto& element : args->elements) {
      added += set->Add(element) ? 1 : 0;
    }
    return added;
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace
}  // namespace redis_simple::command::t_set
