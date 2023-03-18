#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "server/conn_handler/conn_handler.h"
#include "server/connection/connection.h"
#include "server/event_loop/ae.h"
#include "server/networking/networking.h"

namespace redis_simple {
void writeHandler(connection::Connection* conn);

struct ConnWriteHandler : public connection::ConnHandler {
  void handle(connection::Connection* conn) { writeHandler(conn); }
};

void writeHandler(connection::Connection* conn) {
  printf("write to %d\n", conn->getFd());

  // const char* content = "test write\n";
  // ssize_t n = conn->connSyncWrite(content, 12, 1000000);
  std::vector<std::string> args{"1", "2"};
  const RedisCommand& cmd = redis_simple::RedisCommand("Test newline\n", args);
  networking::sendCommand(conn, &cmd);
  printf("write handler called\n");
}

void run() {
  std::unique_ptr<const ae::AeEventLoop> el = ae::AeEventLoop::initEventLoop();
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
      std::unique_ptr<ConnWriteHandler>(new ConnWriteHandler());
  conn->setWriteHandler(move(handler));

  el->aeMain();
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
