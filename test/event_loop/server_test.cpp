#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "server/conn_handler/conn_handler.h"
#include "server/connection/connection.h"
#include "server/event_loop/ae.h"
#include "server/networking/networking.h"

namespace redis_simple {
void acceptHandler(connection::Connection* conn);
void readHandler(connection::Connection* conn);

struct ConnAcceptHandler : public connection::ConnHandler {
  void handle(connection::Connection* conn) { acceptHandler(conn); }
};

struct ConnReadHandler : public connection::ConnHandler {
  void handle(connection::Connection* conn) { readHandler(conn); }
};

void readHandler(connection::Connection* conn) {
  printf("read from %d\n", conn->getFd());
  std::string buffer;

  // ssize_t n = conn->connSyncRead(buffer, 1000000);
  // printf("read %zu bytes\n", n);
  // printf("read content: %s\n",  string(buffer));

  // string res = handler->syncReceiveResponse(conn);
  std::string res = networking::syncReceiveRespline(conn);
  printf("receive resp: %s\n", res.c_str());

  buffer = "";
  res = networking::syncReceiveRespline(conn);

  printf("receive resp after newline: %s\n", res.c_str());
}

void acceptHandler(connection::Connection* conn) {
  std::string ip;
  int port = 0;

  int fd = conn->getFd();
  conn->accept(&ip, &port);

  printf("accept %s:%d\n", ip.c_str(), port);

  // conn->getEventLoop()->aeDeleteFileEvent(fd, AeFlags::aeReadable);
  // close(fd);
  std::unique_ptr<ConnReadHandler> handler =
      std::unique_ptr<ConnReadHandler>(new ConnReadHandler());
  conn->setReadHandler(move(handler));
}

void run() {
  std::unique_ptr<const ae::AeEventLoop> el = ae::AeEventLoop::initEventLoop();
  connection::Connection* conn =
      new connection::Connection({.fd = -1, .loop = el.get()});
  if (conn->listen("localhost", 8081) == connection::StatusCode::c_err) {
    printf("listen failed\n");
    return;
  }

  std::string ip;
  int port = 0;
  if (conn->accept(&ip, &port) == connection::StatusCode::c_err) {
    printf("accept failed\n");
    return;
  }

  std::unique_ptr<ConnAcceptHandler> handler =
      std::unique_ptr<ConnAcceptHandler>(new ConnAcceptHandler());

  printf("set read hander\n");
  conn->setReadHandler(std::move(handler));

  el->aeMain();

  return;
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
