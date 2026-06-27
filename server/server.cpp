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
Server::Server() : db_(db::RedisDb::Init()), el_(ae::EventLoop::Create()) {}

Server* const Server::Get() {
  static std::unique_ptr<Server> server;
  if (!server) {
    server = std::unique_ptr<Server>(new Server());
  }
  return server.get();
}

void Server::Run(const std::string& ip, const int& port) {
  connection::Context ctx;
  ctx.event_loop = el_;
  ctx.fd = -1;
  connection::Connection conn(ctx);
  const connection::AddressInfo addr_info(ip, port);
  if (conn.BindAndListen(addr_info) == connection::ConnectionStatus::kError) {
    return;
  }
  fd_ = conn.Fd();
  AcceptConnHandler();
  ae::TimeEventImpl<Server>::TimeCallback time_proc =
      [](long long id, Server* server) { return server->ServerCron(); };
  el_->CreateTimeEvent(
      ae::TimeEventImpl<Server>::Create(time_proc, nullptr, this));
  el_->Run();
}

bool Server::RemoveClient(Client* c) {
  auto it = std::find(clients_.begin(), clients_.end(), c);
  if (it != clients_.end()) {
    clients_.erase(it);
    delete c;
    c = nullptr;
    return true;
  }
  return false;
}

void Server::AcceptConnHandler() {
  auto* file_event = ae::FileEventImpl<Server>::Create(
      networking::AcceptHandler, nullptr, this, ae::EventFlag::kReadable);
  if (el_->CreateFileEvent(fd_, file_event) == ae::EventLoopStatus::kError) {
    RS_LOG_DEBUG("error in adding client creation file event\n");
  }
}

int Server::ServerCron() {
  ActiveExpireCycle();
  return 1;
}
}  // namespace redis_simple
