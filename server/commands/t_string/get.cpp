#include "server/commands/t_string/get.h"

#include "server/client.h"
#include "server/reply/reply.h"

namespace redis_simple {
namespace command {
namespace t_string {
void GetCommand::Exec(Client* const client) const {
  printf("get command called\n");
  StrArgs args;
  if (ParseArgs(client->CmdArgs(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    return;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    const db::RedisObj* robj = Get(db, &args);
    if (!robj) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
      return;
    }
    try {
      const std::string& s = robj->String();
      client->AddReply(reply::FromBulkString(s));
    } catch (const std::exception& e) {
      printf("catch type exception %s", e.what());
      client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
    }
  } else {
    printf("db pointer expired\n");
    client->AddReply(reply::FromInt64(reply::ReplyStatus::replyErr));
  }
}

int GetCommand::ParseArgs(const std::vector<std::string>& args,
                          StrArgs* str_args) const {
  if (args.empty()) {
    printf("invalid args\n");
    return -1;
  }
  str_args->key = args[0];
  return 0;
}

const db::RedisObj* GetCommand::Get(std::shared_ptr<const db::RedisDb> db,
                                    const StrArgs* args) const {
  if (!db || !args) {
    return nullptr;
  }
  const db::RedisObj* obj = db->LookupKey(args->key);
  if (obj && obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingString) {
    return nullptr;
  }
  return obj;
}
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
