#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "event_loop/ae.h"
#include "server/conn_handler/conn_handler.h"
#include "server/connection/connection.h"
#include "server/networking/networking.h"
#include "server/networking/redis_cmd.h"

namespace redis_simple {
void HandleWrite(connection::Connection* conn);
void HandleRead(connection::Connection* conn);

int n = 0;
struct ConnWriteHandler : public connection::ConnHandler {
  void Handle(connection::Connection* conn) { HandleWrite(conn); }
};

struct ConnReadHandler : public connection::ConnHandler {
  void Handle(connection::Connection* conn) { HandleRead(conn); }
};

void HandleWrite(connection::Connection* conn) {
  if (conn->State() != connection::ConnState::connStateConnected) {
    printf("invalid connection state\n");
    exit(-1);
    return;
  }

  std::vector<std::string> args{"key", "val", "2000"};
  const networking::RedisCommand& setCmd =
      networking::RedisCommand("SET", args);
  const networking::RedisCommand& getCmd =
      networking::RedisCommand("GET", {"key"});
  const networking::RedisCommand& delCmd =
      networking::RedisCommand("DEL", {"key"});

  networking::SendCommand(conn, &setCmd);
  networking::SendCommand(conn, &getCmd);
  networking::SendCommand(conn, &delCmd);
  networking::SendCommand(conn, &delCmd);

  if (++n >= 1) {
    conn->SetWriteHandler(nullptr);
  }

  std::unique_ptr<ConnReadHandler> rhandler =
      std::make_unique<ConnReadHandler>();
  conn->SetReadHandler(std::move(rhandler));
  printf("write handler called\n");
}

void HandleRead(connection::Connection* conn) {
  printf("read resp from %d\n", conn->Fd());

  std::string s;
  int n = conn->Read(s);

  printf("read %d\n", n);
  printf("receive response: %s\n", s.c_str());
  conn->SetReadHandler(nullptr);
}

void Run() {
  std::shared_ptr<ae::AeEventLoop> el(ae::AeEventLoop::InitEventLoop());
  connection::Connection* conn =
      new connection::Connection({.fd = -1, .loop = el});
  connection::StatusCode r =
      conn->Connect("localhost", 8081, "localhost", 8080);
  printf("conn result %d\n", r);
  if (r == connection::StatusCode::c_err) {
    printf("connection failed\n");
    return;
  }

  std::unique_ptr<ConnWriteHandler> handler =
      std::make_unique<ConnWriteHandler>();
  conn->SetWriteHandler(std::move(handler));

  el->AeMain();
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
