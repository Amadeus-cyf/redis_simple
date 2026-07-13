#include "server.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <algorithm>
#include <any>
#include <utility>

#include "client_connection/client_connection.h"
#include "db/db.h"
#include "event_loop/file_event.h"
#include "event_loop/time_event.h"
#include "expire.h"
#include "logging/logger.h"

namespace redis_simple {
Server::Server()
    : loop_(event_loop::Loop::Create()), db_(db::RedisDb::Create()) {}

Server* Server::Get() {
  static Server server;
  return &server;
}

bool Server::Run(const std::string& ip, int port) {
  if (loop_ == nullptr || db_ == nullptr) {
    return false;
  }
  connection::Context ctx;
  ctx.loop = loop_.get();
  ctx.fd = -1;
  connection::Connection conn(ctx);
  const connection::AddressInfo addr_info(ip, port);
  if (conn.BindAndListen(addr_info) == connection::ConnectionStatus::kError) {
    return false;
  }
  fd_ = conn.Descriptor();
  if (!InstallAcceptCallback()) {
    fd_ = -1;
    return false;
  }
  loop_->CreateTimeEvent(event_loop::TimeEvent::Create(
      [](int64_t) { return ServerCron(); }, nullptr));
  loop_->Run();
  loop_->DeleteFileEvent(fd_, event_loop::EventFlag::kReadable);
  fd_ = -1;
  return true;
}

void Server::Stop() {
  if (loop_ != nullptr) {
    loop_->Stop();
  }
}

bool Server::RemoveClient(Client* c) {
  if (c == nullptr) {
    return false;
  }
  c->Free();
  loop_->Defer([this, c] {
    const auto it =
        std::find_if(clients_.begin(), clients_.end(),
                     [c](const auto& client) { return client.get() == c; });
    if (it != clients_.end()) {
      clients_.erase(it);
    }
  });
  return true;
}

bool Server::InstallAcceptCallback() {
  auto file_event = event_loop::FileEvent::Create(
      client_connection::AcceptConnectionCallback, nullptr, this,
      event_loop::ToInt(event_loop::EventFlag::kReadable));
  if (loop_->CreateFileEvent(fd_, std::move(file_event)) ==
      event_loop::Status::kError) {
    RS_LOG_DEBUG("error in adding client creation file event\n");
    return false;
  }
  return true;
}

int Server::ServerCron() {
  ActiveExpireCycle();
  return 1;
}
}  // namespace redis_simple
