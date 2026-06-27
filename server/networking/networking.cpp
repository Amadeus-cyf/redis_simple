#include "networking.h"

#include <cstddef>

#include "server/client.h"
#include "server/networking/conn_handler/conn_handler.h"
#include "server/networking/redis_cmd.h"
#include "server/server.h"

namespace redis_simple::networking {
namespace {
bool SendString(const connection::Connection* conn, const std::string& cmd) {
  ssize_t ret =
      conn->SyncWrite(cmd.c_str(), cmd.length(), static_cast<long>(1 * 1000));
  return ret == cmd.size();
}

bool SendStringInline(const connection::Connection* conn, std::string s) {
  s.push_back('\n');
  return SendString(conn, s);
}
}  // namespace

bool SendCommand(const connection::Connection* conn, const RedisCommand* cmd) {
  return SendStringInline(conn, cmd->String());
}

ae::EventHandlerStatus AcceptHandler(ae::EventLoop* el, int fd, Server* server,
                                     int mask) {
  if (server == nullptr) {
    RS_LOG_DEBUG("invalid server / event loop\n");
    return ae::EventHandlerStatus::kError;
  }
  // Keep the connection tied to the server-owned shared EventLoop, not the raw
  // callback pointer.
  connection::Context ctx;
  ctx.event_loop = server->EventLoop();
  ctx.fd = fd;
  auto* conn = new connection::Connection(ctx);
  conn->SetState(connection::ConnectionState::kAccepting);
  connection::AddressInfo addr_info;
  if (conn->Accept(&addr_info) == connection::ConnectionStatus::kError) {
    RS_LOG_DEBUG("connection accept failed\n");
    return ae::EventHandlerStatus::kError;
  }
  if (conn->State() != connection::ConnectionState::kConnected) {
    RS_LOG_DEBUG("invalid connection state\n");
    return ae::EventHandlerStatus::kError;
  }
  RS_LOG_DEBUG("accept connection from %s:%d with fd = %d\n",
               addr_info.ip.c_str(), addr_info.port, conn->Fd());
  RS_LOG_DEBUG("start create client\n");
  Client* client = Client::Create(conn);
  conn->SetPrivateData(client);
  if (!conn->SetReadHandler(CreateConnHandler(
          connection::ConnectionHandlerType::kReadQueryFromClient))) {
    RS_LOG_DEBUG("AcceptHandler: failed to set the read handler\n");
    return ae::EventHandlerStatus::kError;
  }
  server->AddClient(client);
  return ae::EventHandlerStatus::kOk;
}
}  // namespace redis_simple::networking
