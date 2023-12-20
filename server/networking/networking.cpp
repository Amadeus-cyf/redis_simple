#include "networking.h"

#include "server/client.h"
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

/* for testing */
static const std::string& ErrorRecvResp = "+error";
bool SendCommand(const connection::Connection* conn, const RedisCommand* cmd) {
  return SendStringInline(conn, cmd->String());
}

ae::AeEventStatus AcceptHandler(ae::AeEventLoop* el, int fd, Server* server,
                                int mask) {
  std::string dest_ip;
  int dest_port;
  if (!server) {
    printf("invalid server / event loop\n");
    return ae::AeEventStatus::aeEventErr;
  }
  const connection::Context& ctx = {.fd = fd, .loop = el};
  connection::Connection* conn = new connection::Connection(ctx);
  conn->SetState(connection::ConnState::connStateAccepting);
  if (conn->Accept(&dest_ip, &dest_port) == connection::StatusCode::c_err) {
    printf("connection accept failed\n");
    return ae::AeEventStatus::aeEventErr;
  }
  if (conn->State() != connection::ConnState::connStateConnected) {
    printf("invalid connection state\n");
    return ae::AeEventStatus::aeEventErr;
  }
  // create client based on the connection
  printf("accept connection from %s:%d with fd = %d\n", dest_ip.c_str(),
         dest_port, conn->Fd());
  printf("start create client\n");
  Client* client = Client::create(conn);
  conn->SetPrivateData(client);
  // install the read handler for the client connection
  conn->SetReadHandler(connection::ConnHandler::Create(
      connection::ConnHandlerType::readQueryFromClient));
  server->AddClient(client);
  return ae::AeEventStatus::aeEventOK;
}
}  // namespace networking
}  // namespace redis_simple
