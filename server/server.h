#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "connection/connection.h"
#include "event_loop/loop.h"
#include "server/client.h"

namespace redis_simple {
class Server {
 public:
  static Server* Get();
  bool Run(const std::string& ip, int port);
  void Stop();
  event_loop::Loop* Loop() { return loop_.get(); }
  db::RedisDb* Db() { return db_.get(); }
  void AddClient(std::unique_ptr<Client> client) {
    clients_.push_back(std::move(client));
  }
  bool RemoveClient(Client* c);
  const std::vector<std::unique_ptr<Client>>& Clients() const {
    return clients_;
  }
  ~Server() = default;

 private:
  Server();
  bool InstallAcceptCallback();
  static int ServerCron();
  int fd_{};
  std::unique_ptr<event_loop::Loop> loop_;
  std::vector<std::unique_ptr<Client>> clients_;
  std::unique_ptr<db::RedisDb> db_;
};
}  // namespace redis_simple
