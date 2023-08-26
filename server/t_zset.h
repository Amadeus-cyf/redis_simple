#pragma once

namespace redis_simple {
class Client;
namespace t_cmd {
void zAddCommand(Client* const client);
void zRemCommand(Client* const client);
void zRankCommand(Client* const client);
}  // namespace t_cmd
}  // namespace redis_simple
