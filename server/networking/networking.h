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
bool sendStringInline(const connection::Connection* conn, std::string s);
bool sendString(const connection::Connection* conn, const std::string& s);
std::string syncReceiveResponse(const connection::Connection* conn);
std::string syncReceiveResponse(const connection::Connection* conn,
                                long timeout);
std::string syncReceiveRespline(const connection::Connection* conn);
ae::AeEventStatus acceptHandler(ae::AeEventLoop* el, int fd, Server* server,
                                int mask);
}  // namespace networking
}  // namespace redis_simple
