#include "client_connection.h"

#include <memory>
#include <utility>

#include "logging/logger.h"
#include "server/client.h"
#include "server/client_connection/callback.h"
#include "server/server.h"

namespace redis_simple::client_connection {
event_loop::CallbackStatus AcceptConnectionCallback(event_loop::Loop* /*loop*/,
                                                    int fd, Server* server,
                                                    int /*mask*/) {
  if (server == nullptr) {
    RS_LOG_DEBUG("invalid server / event loop\n");
    return event_loop::CallbackStatus::kError;
  }
  connection::Context ctx;
  ctx.loop = server->Loop();
  ctx.fd = -1;
  auto conn = std::make_unique<connection::Connection>(ctx);
  conn->SetState(connection::ConnectionState::kAccepting);
  connection::AddressInfo addr_info;
  if (conn->Accept(fd, &addr_info) == connection::ConnectionStatus::kError) {
    RS_LOG_DEBUG("connection accept failed\n");
    return event_loop::CallbackStatus::kError;
  }
  if (conn->State() != connection::ConnectionState::kConnected) {
    RS_LOG_DEBUG("invalid connection state\n");
    return event_loop::CallbackStatus::kError;
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
    return event_loop::CallbackStatus::kError;
  }
  server->AddClient(std::move(client));
  return event_loop::CallbackStatus::kOk;
}
}  // namespace redis_simple::client_connection
