#include "client_connection.h"

#include <cstddef>
#include <memory>

#include "server/client.h"
#include "server/client_connection/callback.h"
#include "server/client_connection/redis_cmd.h"
#include "server/server.h"

namespace redis_simple::client_connection {
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
  connection::Context ctx;
  ctx.event_loop = server->EventLoop();
  ctx.fd = fd;
  auto conn = std::make_unique<connection::Connection>(ctx);
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
               addr_info.ip.c_str(), addr_info.port, conn->Descriptor());
  RS_LOG_DEBUG("start create client\n");
  auto client = Client::Create(std::move(conn));
  Client* client_ptr = client.get();
  client_ptr->Connection()->SetPrivateData(client_ptr);
  if (!client_ptr->Connection()->SetReadCallback(
          CreateCallback(CallbackType::kReadQuery))) {
    RS_LOG_DEBUG("AcceptConnectionCallback: failed to set the read callback\n");
    return ae::EventCallbackStatus::kError;
  }
  server->AddClient(std::move(client));
  return ae::EventCallbackStatus::kOk;
}
}  // namespace redis_simple::client_connection
