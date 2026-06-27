#pragma once

namespace redis_simple {
class Client;
}  // namespace redis_simple

namespace redis_simple::command::t_set {
void ExecuteSRem(Client* client);
}  // namespace redis_simple::command::t_set
