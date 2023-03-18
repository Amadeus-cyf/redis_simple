#include "networking.h"

#include "server/client.h"
#include "server/server.h"

namespace redis_simple {
namespace networking {
bool sendCommand(const connection::Connection* conn, const RedisCommand* cmd) {
  return sendStringInline(conn, cmd->toString());
}

bool sendStringInline(const connection::Connection* conn, std::string s) {
  s.push_back('\n');
  return sendString(conn, s);
}

bool sendString(const connection::Connection* conn, const std::string& cmd) {
  ssize_t ret = conn->connSyncWrite(cmd.c_str(), cmd.length(), 1 * 1000);
  return ret == cmd.size();
}

std::string syncReceiveResponse(const connection::Connection* conn) {
  return syncReceiveResponse(conn, 1 * 1000);
}

std::string syncReceiveResponse(const connection::Connection* conn,
                                long timeout) {
  std::string reply;
  ssize_t r = conn->connSyncRead(reply, timeout);
  return r < 0 ? ErrorRecvResp : reply;
}

std::string syncReceiveRespline(const connection::Connection* conn) {
  std::string reply;
  ssize_t r = conn->connSyncReadline(reply, 1 * 1000);
  return r < 0 ? ErrorRecvResp : reply;
}

ae::AeEventStatus acceptHandler(const ae::AeEventLoop* el, int fd,
                                Server* server, int mask) {
  std::string dest_ip;
  int dest_port;
  if (!server || !server->getEventLoop()) {
    printf("invalid server / event loop\n");
    return ae::AeEventStatus::aeEventErr;
  }
  const connection::Context& ctx = {.fd = fd, .loop = el};
  connection::Connection* conn = new connection::Connection(ctx);
  conn->setState(connection::ConnState::connStateAccepting);
  if (conn->accept(&dest_ip, &dest_port) == connection::StatusCode::c_err) {
    printf("connection accept failed\n");
    return ae::AeEventStatus::aeEventErr;
  }
  if (conn->getState() != connection::ConnState::connStateConnected) {
    printf("invalid connection state\n");
    return ae::AeEventStatus::aeEventErr;
  }
  printf("accept connection from %s:%d with fd = %d\n", dest_ip.c_str(),
         dest_port, conn->getFd());
  printf("start create client\n");
  Client* client = new Client(conn);
  conn->setPrivateData(client);
  // set read handler for client conn
  conn->setReadHandler(connection::ConnHandler::create(
      connection::ConnHandlerType::readQueryFromClient));
  server->addClient(client);
  return ae::AeEventStatus::aeEventOK;
}
}  // namespace networking
}  // namespace redis_simple
