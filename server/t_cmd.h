#pragma once

#include <string>

namespace redis_simple {
class Client;

namespace t_cmd {
using RedisCmdProc = void (*)(Client*);
RedisCmdProc getRedisCmdProc(const std::string& cmd);
void addReplyToClient(Client* client, const std::string& reply);
}  // namespace t_cmd
}  // namespace redis_simple
