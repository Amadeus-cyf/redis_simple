#pragma once

#include "event_loop/ae.h"
#include "server/networking/redis_cmd.h"

namespace redis_simple {
class Server;

namespace connection {
class Connection;
}

namespace networking {
bool SendCommand(const connection::Connection* conn, const RedisCommand* cmd);
ae::AeEventStatus AcceptHandler(ae::AeEventLoop* el, int fd, Server* server,
                                int mask);
}  // namespace networking
}  // namespace redis_simple
