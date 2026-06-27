#pragma once

#include "event_loop/ae.h"
#include "server/client_connection/redis_cmd.h"

namespace redis_simple {
class Server;
}  // namespace redis_simple

namespace redis_simple::connection {
class Connection;
}  // namespace redis_simple::connection

namespace redis_simple::client_connection {
bool SendCommand(const connection::Connection* conn, const RedisCommand* cmd);
ae::EventCallbackStatus AcceptConnectionCallback(ae::EventLoop* el, int fd,
                                                 Server* server, int mask);
}  // namespace redis_simple::client_connection
