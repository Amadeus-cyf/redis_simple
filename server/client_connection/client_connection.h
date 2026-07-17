#pragma once

#include "event_loop/loop.h"

namespace redis_simple {
class Server;
}  // namespace redis_simple

namespace redis_simple::client_connection {
event_loop::CallbackStatus AcceptConnectionCallback(event_loop::Loop* loop,
                                                    int fd, Server* server,
                                                    int mask);
}  // namespace redis_simple::client_connection
