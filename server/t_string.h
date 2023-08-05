#pragma once

#include <string>
#include <unordered_map>

#include "server/db/db.h"

namespace redis_simple {
class Client;

namespace t_cmd {
void setCommand(Client* const client);
void getCommand(Client* const client);
void delCommand(Client* const client);
}  // namespace t_cmd
}  // namespace redis_simple
