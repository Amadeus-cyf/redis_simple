#pragma once

namespace redis_simple {
class Client;

namespace command::t_string {
void ExecuteSet(Client* client);
}  // namespace command::t_string
}  // namespace redis_simple
