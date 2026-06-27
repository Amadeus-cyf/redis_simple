#pragma once

namespace redis_simple {
class Client;

namespace command::t_zset {
void ExecuteZCard(Client* client);
}  // namespace command::t_zset
}  // namespace redis_simple
