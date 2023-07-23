#include "t_cmd.h"

#include "server/client.h"
#include "t_string.h"

namespace redis_simple {
namespace t_cmd {
namespace {
std::unordered_map<std::string, RedisCmdProc>& getCmdMapping() {
  static std::unordered_map<std::string, RedisCmdProc> cmdMap;
  if (cmdMap.size() > 0) {
    return cmdMap;
  }
  cmdMap["SET"] = setCommand;
  cmdMap["GET"] = getCommand;
  cmdMap["DEL"] = delCommand;
  return cmdMap;
}
}  // namespace

void addReplyToClient(Client* client, const std::string& reply) {
  client->addReply(reply);
}

RedisCmdProc getRedisCmdProc(const std::string& cmd) {
  return getCmdMapping()[cmd];
}
}  // namespace t_cmd
}  // namespace redis_simple
