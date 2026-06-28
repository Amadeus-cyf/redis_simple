#include "server.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <algorithm>
#include <any>
#include <utility>

#include "client_connection/client_connection.h"
#include "db/db.h"
#include "event_loop/ae_file_event.h"
#include "event_loop/ae_time_event.h"
#include "expire.h"

namespace redis_simple {
Server::Server() : db_(db::RedisDb::Init()), el_(ae::EventLoop::Create()) {}

Server* Server::Get() {
  static Server server;
  return &server;
}

bool Server::Run(const std::string& ip, int port) {
  connection::Context ctx;
  ctx.event_loop = el_.get();
  ctx.fd = -1;
  connection::Connection conn(ctx);
  const connection::AddressInfo addr_info(ip, port);
  if (conn.BindAndListen(addr_info) == connection::ConnectionStatus::kError) {
    return false;
  }
  fd_ = conn.Descriptor();
  InstallAcceptCallback();
  el_->CreateTimeEvent(ae::TimeEvent::Create(
      [this](long long id) { return ServerCron(); }, nullptr));
  el_->Run();
  return true;
}

void Server::Stop() {
  if (el_ != nullptr) {
    el_->Stop();
  }
}

bool Server::RemoveClient(Client* c) {
  auto it = std::find_if(clients_.begin(), clients_.end(),
                         [c](const auto& client) { return client.get() == c; });
  if (it != clients_.end()) {
    clients_.erase(it);
    return true;
  }
  return false;
}

void Server::InstallAcceptCallback() {
  auto file_event =
      ae::FileEvent::Create(client_connection::AcceptConnectionCallback,
                            nullptr, this, ae::ToInt(ae::EventFlag::kReadable));
  if (el_->CreateFileEvent(fd_, std::move(file_event)) ==
      ae::EventLoopStatus::kError) {
    RS_LOG_DEBUG("error in adding client creation file event\n");
  }
}

int Server::ServerCron() {
  ActiveExpireCycle();
  return 1;
}
}  // namespace redis_simple
