#include "server/commands/t_set/scard.h"

#include <limits>
#include <string>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_set {
namespace {
struct SCardArgs {
  std::string key;
};
int ParseArgs(const std::vector<std::string>& args, SCardArgs* scard_args);
ssize_t SCard(db::RedisDb* redis_db, const SCardArgs* args);
}  // namespace

void ExecuteSCard(Client* const client) {
  SCardArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    ssize_t result = SCard(redis_db, &args);
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

int ParseArgs(const std::vector<std::string>& args,
              SCardArgs* const scard_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  scard_args->key = args[0];
  return 0;
}

ssize_t SCard(db::RedisDb* redis_db, const SCardArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kSet) {
    return -1;
  }
  try {
    const auto* set = obj->Set();
    const size_t size = set->Size();
    if (size > static_cast<size_t>(std::numeric_limits<ssize_t>::max())) {
      return -1;
    }
    return static_cast<ssize_t>(size);
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception %s", e.what());
    return -1;
  }
}
}  // namespace
}  // namespace redis_simple::command::t_set
