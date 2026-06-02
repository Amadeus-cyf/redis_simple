#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "connection/conn_handler.h"
#include "connection/connection.h"
#include "event_loop/ae.h"
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
    RS_LOG_DEBUG("mock client connection failed: invalid connection state\n");
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

  bool r = networking::SendCommand(conn, &setCmd);
  networking::SendCommand(conn, &getCmd);
  networking::SendCommand(conn, &delCmd);
  networking::SendCommand(conn, &delCmd);

  if (++n >= 1) {
    conn->SetWriteHandler(nullptr);
  }

  std::unique_ptr<ConnReadHandler> rhandler =
      std::make_unique<ConnReadHandler>();
  if (!conn->SetReadHandler(std::move(rhandler))) {
    RS_LOG_DEBUG("failed to set read handler\n");
    return;
  }
  RS_LOG_DEBUG("write handler called\n");
}

void HandleRead(connection::Connection* conn) {
  RS_LOG_DEBUG("read resp from %d\n", conn->Fd());

  std::string s;
  int nread = conn->BatchRead(s);
  RS_LOG_DEBUG("read %d\n", nread);
  RS_LOG_DEBUG("receive response: %s\n", s.c_str());
  conn->SetReadHandler(nullptr);
}

void Run() {
  std::shared_ptr<ae::AeEventLoop> el(ae::AeEventLoop::InitEventLoop());
  connection::Context ctx;
  ctx.event_loop = el;
  ctx.fd = -1;
  connection::Connection* conn = new connection::Connection(ctx);
  const connection::AddressInfo remote("localhost", 8080);
  const connection::AddressInfo local("localhost", 8081);
  connection::StatusCode r = conn->BindAndConnect(remote, local);
  RS_LOG_DEBUG("conn result %d\n", r);
  if (r == connection::StatusCode::connStatusErr) {
    RS_LOG_DEBUG("connection failed\n");
    return;
  }
  std::unique_ptr<ConnWriteHandler> handler =
      std::make_unique<ConnWriteHandler>();
  if (!conn->SetWriteHandler(std::move(handler))) {
    RS_LOG_DEBUG("failed to unset the write handler\n");
    return;
  }
  el->AeMain();
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
