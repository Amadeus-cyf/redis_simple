#include "server.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <any>

#include "db/db.h"
#include "event_loop/ae_file_event_impl.h"
#include "event_loop/ae_time_event_impl.h"
#include "expire.h"
#include "networking/networking.h"

namespace redis_simple {
Server::Server()
    : db_(db::RedisDb::Init()), el_(ae::AeEventLoop::InitEventLoop()) {}

Server* Server::Get() {
  static std::unique_ptr<Server> server;
  if (!server) {
    server = std::unique_ptr<Server>(new Server());
  }
  return server.get();
}

void Server::Run(const std::string& ip, const int& port) {
  const connection::Context& ctx = {.fd = -1, .event_loop = el_};
  connection::Connection conn(ctx);
  const connection::AddressInfo addrInfo(ip, port);
  if (conn.BindAndListen(addrInfo) == connection::StatusCode::c_err) {
    return;
  }
  fd_ = conn.Fd();
  AcceptConnHandler();
  ae::AeTimeEventImpl<Server>::aeTimeProc time_proc = [](long long id,
                                                         Server* server) {
    return Get()->ServerCron(id, server);
  };
  el_->AeCreateTimeEvent(
      ae::AeTimeEventImpl<Server>::Create(time_proc, nullptr, this));
  el_->AeMain();
}

void Server::AcceptConnHandler() {
  ae::AeFileEvent* fe = ae::AeFileEventImpl<Server>::Create(
      networking::AcceptHandler, nullptr, this, ae::aeReadable);
  if (el_->AeCreateFileEvent(fd_, fe) < 0) {
    printf("error in adding client creation file event\n");
  }
}

int Server::ServerCron(long long id, std::any clientData) {
  ActiveExpireCycle();
  return 1;
}
}  // namespace redis_simple
