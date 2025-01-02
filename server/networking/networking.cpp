#include "networking.h"

#include "server/client.h"
#include "server/networking/conn_handler/conn_handler.h"
#include "server/networking/redis_cmd.h"
#include "server/server.h"

namespace redis_simple {
namespace networking {
namespace {
bool SendString(const connection::Connection* conn, const std::string& cmd) {
  ssize_t ret = conn->SyncWrite(cmd.c_str(), cmd.length(), 1 * 1000);
  return ret == cmd.size();
}

bool SendStringInline(const connection::Connection* conn, std::string s) {
  s.push_back('\n');
  return SendString(conn, s);
}
}  // namespace

// For testing
static const std::string& ErrorRecvResp = "+error";
bool SendCommand(const connection::Connection* conn, const RedisCommand* cmd) {
  return SendStringInline(conn, cmd->String());
}

ae::AeEventStatus AcceptHandler(ae::AeEventLoop* el, int fd, Server* server,
                                int mask) {
  if (!server) {
    printf("invalid server / event loop\n");
    return ae::AeEventStatus::aeEventErr;
  }
  // Should use server->EventLoop() method to get the shared pointer of event
  // loop instead of using the raw pointer passed as arg.
  const connection::Context& ctx = {
      .fd = fd,
      .event_loop = server->EventLoop(),
  };
  connection::Connection* conn = new connection::Connection(ctx);
  conn->SetState(connection::ConnState::connStateAccepting);
  connection::AddressInfo addrInfo;
  if (conn->Accept(&addrInfo) == connection::StatusCode::connStatusErr) {
    printf("connection accept failed\n");
    return ae::AeEventStatus::aeEventErr;
  }
  if (conn->State() != connection::ConnState::connStateConnected) {
    printf("invalid connection state\n");
    return ae::AeEventStatus::aeEventErr;
  }
  // Create client based on the connection.
  printf("accept connection from %s:%d with fd = %d\n", addrInfo.ip.c_str(),
         addrInfo.port, conn->Fd());
  printf("start create client\n");
  Client* client = Client::Create(conn);
  conn->SetPrivateData(client);
  // Install the read handler for the client connection.
  if (!conn->SetReadHandler(CreateConnHandler(
          connection::ConnHandlerType::readQueryFromClient))) {
    printf("AcceptHandler: failed to set the read handler\n");
    return ae::AeEventStatus::aeEventErr;
  }
  server->AddClient(client);
  return ae::AeEventStatus::aeEventOK;
}
}  // namespace networking
}  // namespace redis_simple
