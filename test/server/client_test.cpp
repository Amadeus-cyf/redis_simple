#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "server/conn_handler/conn_handler.h"
#include "server/connection/connection.h"
#include "server/event_loop/ae.h"
#include "server/networking/networking.h"

namespace redis_simple {
void writeHandler(connection::Connection* conn);
void readHandler(connection::Connection* conn);

int n = 0;
struct ConnWriteHandler : public connection::ConnHandler {
  void handle(connection::Connection* conn) { writeHandler(conn); }
};

struct ConnReadHandler : public connection::ConnHandler {
  void handle(connection::Connection* conn) { readHandler(conn); }
};

void writeHandler(connection::Connection* conn) {
  if (conn->getState() != connection::ConnState::connStateConnected) {
    printf("invalid connection state\n");
    exit(-1);
    return;
  }

  printf("write to %d\n", conn->getFd());
  std::vector<std::string> args{"key", "val", "2000"};
  const RedisCommand& setCmd = RedisCommand("SET", args);
  const RedisCommand& getCmd = RedisCommand("GET", {"key"});
  const RedisCommand& delCmd = RedisCommand("DEL", {"key"});

  networking::sendCommand(conn, &setCmd);
  networking::sendCommand(conn, &getCmd);
  networking::sendCommand(conn, &delCmd);
  networking::sendCommand(conn, &delCmd);

  if (++n >= 1) {
    conn->setWriteHandler(nullptr);
  }

  std::unique_ptr<ConnReadHandler> rhandler =
      std::make_unique<ConnReadHandler>();
  conn->setReadHandler(move(rhandler));

  printf("write handler called\n");
}

void readHandler(connection::Connection* conn) {
  printf("read resp from %d\n", conn->getFd());

  std::string s;
  int n = conn->connRead(s);

  printf("read %d\n", n);
  printf("receive response: %s\n", s.c_str());
  conn->setReadHandler(nullptr);
}

void run() {
  std::unique_ptr<ae::AeEventLoop> el = ae::AeEventLoop::initEventLoop();
  connection::Connection* conn =
      new connection::Connection({.fd = -1, .loop = el.get()});
  connection::StatusCode r =
      conn->connect("localhost", 8081, "localhost", 8080);
  printf("conn result %d\n", r);
  if (r == connection::StatusCode::c_err) {
    printf("connection failed\n");
    return;
  }

  std::unique_ptr<ConnWriteHandler> handler =
      std::make_unique<ConnWriteHandler>();
  conn->setWriteHandler(move(handler));

  el->aeMain();
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
