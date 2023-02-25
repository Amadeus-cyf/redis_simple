#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "src/conn_handler/conn_handler.h"
#include "src/connection/connection.h"
#include "src/event_loop/ae.h"
#include "src/networking/networking.h"

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
  printf("write to %d\n", conn->getFd());

  std::vector<std::string> args{"key", "val", "1000"};
  const RedisCommand& setCmd = RedisCommand("SET", args);
  const RedisCommand& getCmd = RedisCommand("GET", {"key"});
  // const RedisCommand& delCmd = RedisCommand("DEL", {"key"});

  networking::sendCommand(conn, &setCmd);
  networking::sendCommand(conn, &getCmd);
  // networking::sendCommand(conn, &delCmd);
  // networking::sendCommand(conn, &delCmd);

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
}

void run() {
  ae::AeEventLoop* el = ae::AeEventLoop::initEventLoop();

  connection::Connection* conn = new connection::Connection();

  conn->bindEventLoop(el);
  printf("conn result %d\n",
         conn->connect("localhost", 8080, "localhost", 8081));

  std::unique_ptr<ConnWriteHandler> handler =
      std::make_unique<ConnWriteHandler>();
  conn->setWriteHandler(move(handler));

  el->aeMain();
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
