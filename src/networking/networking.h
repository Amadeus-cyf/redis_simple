#pragma once

#include "src/event_loop/ae.h"
#include "src/redis_cmd/redis_cmd.h"

namespace redis_simple {
namespace connection {
class Connection;
}

namespace networking {
static const std::string ErrorRecvResp;
bool sendCommand(connection::Connection* conn, const RedisCommand* cmd);
bool sendStringInline(connection::Connection* conn, std::string s);
bool sendString(connection::Connection* conn, const std::string& s);
std::string syncReceiveResponse(connection::Connection* conn);
std::string syncReceiveRespline(connection::Connection* conn);
ae::AeEventStatus acceptHandler(int fd, void* clientData, int mask);
}  // namespace networking
}  // namespace redis_simple
