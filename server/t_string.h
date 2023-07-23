#pragma once

#include <string>
#include <unordered_map>

#include "server/db/db.h"

namespace redis_simple {
class Client;

namespace t_cmd {
void setCommand(Client* client);
void getCommand(Client* client);
void delCommand(Client* client);
}  // namespace t_cmd
}  // namespace redis_simple
