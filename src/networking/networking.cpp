#include "networking.h"

#include "src/client.h"
#include "src/server.h"

namespace redis_simple {
namespace networking {
bool sendCommand(connection::Connection* conn, const RedisCommand* cmd) {
  return sendStringInline(conn, cmd->toString());
}

bool sendStringInline(connection::Connection* conn, std::string s) {
  s.push_back('\n');
  return sendString(conn, s);
}

bool sendString(connection::Connection* conn, const std::string& cmd) {
  ssize_t ret = conn->connSyncWrite(cmd.c_str(), cmd.length(), 1 * 1000);
  return ret >= 0;
}

std::string syncReceiveResponse(connection::Connection* conn) {
  std::string reply;
  ssize_t r = conn->connSyncRead(reply, 1 * 1000);
  return r < 0 ? ErrorRecvResp : reply;
}

std::string syncReceiveRespline(connection::Connection* conn) {
  std::string reply;
  ssize_t r = conn->connSyncReadline(reply, 1 * 1000);
  return r < 0 ? ErrorRecvResp : reply;
}

ae::AeEventStatus acceptHandler(int fd, void* clientData, int mask) {
  std::string dest_ip;
  int dest_port;
  Server* server = static_cast<Server*>(clientData);
  if (!server || !server->getEventLoop()) {
    printf("invalid server / event loop\n");
    return ae::AeEventStatus::aeEventErr;
  }
  connection::Connection* conn = new connection::Connection(fd);
  conn->setState(connection::ConnState::connStateAccepting);
  conn->bindEventLoop(server->getEventLoop());
  if (conn->accept(&dest_ip, &dest_port) == connection::StatusCode::c_err) {
    printf("connection accept failed\n");
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
