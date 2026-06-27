#pragma once

namespace redis_simple {
class Client;
}  // namespace redis_simple

namespace redis_simple::command::t_string {
void ExecuteSet(Client* client);
}  // namespace redis_simple::command::t_string
