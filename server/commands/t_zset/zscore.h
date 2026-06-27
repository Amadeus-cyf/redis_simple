#pragma once

namespace redis_simple {
class Client;

namespace command::t_zset {
void ExecuteZScore(Client* client);
}  // namespace command::t_zset
}  // namespace redis_simple
