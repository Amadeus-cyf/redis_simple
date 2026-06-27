#pragma once

namespace redis_simple {
class Client;

namespace command::t_set {
void ExecuteSCard(Client* client);
}  // namespace command::t_set
}  // namespace redis_simple
