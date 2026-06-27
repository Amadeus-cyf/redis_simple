#pragma once

namespace redis_simple {
class Client;
}  // namespace redis_simple

namespace redis_simple::command::t_zset {
void ExecuteZScore(Client* client);
}  // namespace redis_simple::command::t_zset
