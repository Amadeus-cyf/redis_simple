#include "networking.h"

#include <cstddef>

#include "server/client.h"
#include "server/networking/connection_callback/connection_callback.h"
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

ae::EventCallbackStatus AcceptConnectionCallback(ae::EventLoop* el, int fd,
                                                 Server* server, int mask) {
  if (server == nullptr) {
    RS_LOG_DEBUG("invalid server / event loop\n");
    return ae::EventCallbackStatus::kError;
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
    return ae::EventCallbackStatus::kError;
  }
  if (conn->State() != connection::ConnectionState::kConnected) {
    RS_LOG_DEBUG("invalid connection state\n");
    return ae::EventCallbackStatus::kError;
  }
  RS_LOG_DEBUG("accept connection from %s:%d with fd = %d\n",
               addr_info.ip.c_str(), addr_info.port, conn->Fd());
  RS_LOG_DEBUG("start create client\n");
  Client* client = Client::Create(conn);
  conn->SetPrivateData(client);
  if (!conn->SetReadCallback(CreateConnectionCallback(
          connection::ConnectionCallbackType::kReadQueryFromClient))) {
    RS_LOG_DEBUG("AcceptConnectionCallback: failed to set the read callback\n");
    return ae::EventCallbackStatus::kError;
  }
  server->AddClient(client);
  return ae::EventCallbackStatus::kOk;
}
}  // namespace redis_simple::networking
