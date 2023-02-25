#pragma once

#include <string>
#include <unordered_map>

#include "src/db/db.h"

namespace redis_simple {
class Client;

namespace t_cmd {
using RedisCmdProc = void (*)(Client*);
RedisCmdProc getRedisCmdProc(const std::string& cmd);
void addReplyToClient(Client* client, db::DBStatus status);
void installWriteHandler(Client* client);
void setCommand(Client* client);
void getCommand(Client* client);
void delCommand(Client* client);
}  // namespace t_cmd
}  // namespace redis_simple
