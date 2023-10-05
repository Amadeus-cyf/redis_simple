#include "server/commands/t_string/get.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_string {
void GetCommand::exec(Client* const client) const {
  printf("get command called\n");
  StrArgs args;
  if (parseArgs(client->getArgs(), &args) < 0) {
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  const db::RedisObj* robj = genericGet(client->getDb(), &args);
  if (!robj) {
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  try {
    const std::string& s = robj->getString();
    client->addReply(reply::fromBulkString(s));
  } catch (const std::exception& e) {
    printf("catch type exception %s", e.what());
    client->addReply(reply::fromInt64(reply::ReplyStatus::replyErr));
  }
}

int GetCommand::parseArgs(const std::vector<std::string>& args,
                          StrArgs* str_args) const {
  if (args.empty()) {
    printf("invalid args\n");
    return -1;
  }
  str_args->key = args[0];
  return 0;
}

const db::RedisObj* GetCommand::genericGet(const db::RedisDb* db,
                                           const StrArgs* args) const {
  if (!db || !args) {
    return nullptr;
  }
  const db::RedisObj* obj = db->lookupKey(args->key);
  if (obj &&
      obj->getEncoding() != db::RedisObj::ObjEncoding::objEncodingString) {
    return nullptr;
  }
  return obj;
}
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
