#pragma once

#include <memory>
#include <string>
#include <vector>

#include "connection/connection.h"
#include "event_loop/ae.h"
#include "server/client.h"

namespace redis_simple {
class Server {
 public:
  static Server* const Get();
  void Run(const std::string& ip, const int& port);
  ae::EventLoop* EventLoop() { return el_.get(); }
  db::RedisDb* Db() { return db_.get(); }
  void AddClient(std::unique_ptr<Client> client) {
    clients_.push_back(std::move(client));
  }
  bool RemoveClient(Client* c);
  const std::vector<std::unique_ptr<Client>>& Clients() { return clients_; }
  ~Server() = default;

 private:
  Server();
  void InstallAcceptCallback();
  static int ServerCron();
  int fd_{};
  int flags_{};
  std::unique_ptr<ae::EventLoop> el_;
  std::vector<std::unique_ptr<Client>> clients_;
  std::unique_ptr<db::RedisDb> db_;
};
}  // namespace redis_simple
