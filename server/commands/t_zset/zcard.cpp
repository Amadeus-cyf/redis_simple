#include "server/commands/t_zset/zcard.h"

#include <limits>
#include <string>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::t_zset {
namespace {
struct ZCardArgs {
  std::string key;
};
int ParseArgs(const std::vector<std::string>& args, ZCardArgs* zcard_args);
ssize_t ZCard(db::RedisDb* redis_db, const ZCardArgs* args);
}  // namespace

void ExecuteZCard(Client* const client) {
  ZCardArgs args;
  if (ParseArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    ssize_t result = ZCard(redis_db, &args);
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
              ZCardArgs* const zcard_args) {
  if (args.size() != 1) {
    RS_LOG_DEBUG("invalid number of args\n");
    return -1;
  }
  zcard_args->key = args[0];
  return 0;
}

ssize_t ZCard(db::RedisDb* redis_db, const ZCardArgs* args) {
  const auto* obj = redis_db->LookupKey(args->key);
  if (obj == nullptr) {
    return 0;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
    return -1;
  }
  try {
    const auto* zset = obj->ZSet();
    const size_t size = zset->Size();
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
}  // namespace redis_simple::command::t_zset
