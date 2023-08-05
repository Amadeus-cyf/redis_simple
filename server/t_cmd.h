#pragma once

#include <string>

#include "server/redis_cmd/redis_cmd.h"

namespace redis_simple {
class Client;

namespace t_cmd {
const RedisCommand::RedisCmdProc getRedisCmdProc(const std::string& cmd);
void addReplyToClient(Client* client, const std::string& reply);
}  // namespace t_cmd
}  // namespace redis_simple
