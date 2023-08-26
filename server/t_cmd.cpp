#include "t_cmd.h"

#include "server/client.h"
#include "server/t_string.h"
#include "server/t_zset.h"

namespace redis_simple {
namespace t_cmd {
namespace {
std::unordered_map<std::string, RedisCommand::RedisCmdProc>& getCmdMapping() {
  static std::unordered_map<std::string, RedisCommand::RedisCmdProc> cmdMap;
  if (cmdMap.size() > 0) {
    return cmdMap;
  }
  cmdMap = {
      {"SET", setCommand},   {"GET", getCommand},   {"DEL", delCommand},
      {"ZADD", zAddCommand}, {"ZREM", zRemCommand}, {"ZRANK", zRankCommand},
  };
  return cmdMap;
}
}  // namespace

void addReplyToClient(Client* client, const std::string& reply) {
  client->addReply(reply);
}

const RedisCommand::RedisCmdProc getRedisCmdProc(const std::string& cmd) {
  return getCmdMapping()[cmd];
}
}  // namespace t_cmd
}  // namespace redis_simple
