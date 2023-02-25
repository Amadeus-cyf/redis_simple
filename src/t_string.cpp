#include "t_string.h"

#include "src/client.h"
#include "src/networking/handler/write_client.h"
#include "src/utils/time_utils.h"

namespace redis_simple {
namespace t_cmd {
namespace {
inline std::unordered_map<std::string, RedisCmdProc>& getCmdMapping() {
  static std::unordered_map<std::string, RedisCmdProc> cmdMap;
  cmdMap["SET"] = setCommand;
  cmdMap["GET"] = getCommand;
  cmdMap["DEL"] = delCommand;
  return cmdMap;
}
}  // namespace

void addReplyToClient(Client* client, db::DBStatus status) {
  client->addReply(status == db::DBStatus::dbOK ? "1" : "0");
}

void installWriteHandler(Client* client) {
  if (!client->getConn()->hasWriteHandler()) {
    client->getConn()->setWriteHandler(connection::ConnHandler::create(
        connection::ConnHandlerType::writeReplyToClient));
  }
}

RedisCmdProc getRedisCmdProc(const std::string& cmd) {
  return getCmdMapping()[cmd];
}

void setCommand(Client* client) {
  printf("set command called\n");
  const db::RedisDb* db = client->getDb();
  const RedisCommand* cmd = client->getCmd();
  if (!cmd) {
    printf("no command\n");
    return;
  }
  if (cmd->getArgs().size() < 2) {
    printf("invalid args\n");
    return;
  }
  const std::string& key = cmd->getArgs()[0];
  int64_t expire = 0;
  if (cmd->getArgs().size() >= 3) {
    uint64_t now = utils::getNowInMilliseconds();
    expire = now + std::stoll(cmd->getArgs()[2]);
  }
  const db::RedisObj* val =
      db::RedisObj::createStringRedisObj(cmd->getArgs()[1]);
  installWriteHandler(client);
  addReplyToClient(client, db->setKey(key, val, expire));
}

void getCommand(Client* client) {
  printf("get command called\n");
  const db::RedisDb* db = client->getDb();
  const RedisCommand* cmd = client->getCmd();
  if (!cmd) {
    printf("no command\n");
    return;
  }
  if (cmd->getArgs().empty()) {
    printf("invalid args\n");
    return;
  }
  const std::string& key = cmd->getArgs()[0];
  const db::RedisObj* val = db->lookupKey(key);
  if (!val) {
    addReplyToClient(client, db::DBStatus::dbErr);
    return;
  }
  client->addReply(std::get<std::string>(val->getVal()));
  installWriteHandler(client);
  val->decrRefCount();
}

void delCommand(Client* client) {
  printf("delete command called\n");
  const db::RedisDb* db = client->getDb();
  const RedisCommand* cmd = client->getCmd();
  if (!cmd) {
    printf("no comman\n");
    return;
  }
  if (cmd->getArgs().empty()) {
    printf("invalid args\n");
    return;
  }
  const std::string& key = cmd->getArgs()[0];
  installWriteHandler(client);
  addReplyToClient(client, db->delKey(key));
}
}  // namespace t_cmd
}  // namespace redis_simple
