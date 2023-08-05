#pragma once

namespace redis_simple {
class Client;
namespace t_cmd {
void zaddCommand(Client* const client);
void zdelCommand(Client* const client);
}  // namespace t_cmd
}  // namespace redis_simple
