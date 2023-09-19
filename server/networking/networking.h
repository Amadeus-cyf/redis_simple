#pragma once

#include "event_loop/ae.h"

namespace redis_simple {
class Server;
class RedisCommand;

namespace connection {
class Connection;
}

namespace networking {
bool sendCommand(const connection::Connection* conn, const RedisCommand* cmd);
ae::AeEventStatus acceptHandler(ae::AeEventLoop* el, int fd, Server* server,
                                int mask);
}  // namespace networking
}  // namespace redis_simple
