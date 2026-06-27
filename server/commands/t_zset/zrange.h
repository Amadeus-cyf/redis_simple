#pragma once

namespace redis_simple {
class Client;

namespace command::t_zset {
void ExecuteZRange(Client* client);
}  // namespace command::t_zset
}  // namespace redis_simple
