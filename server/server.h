#pragma once

#include <string>
#include <vector>

#include "event_loop/ae.h"
#include "server/client.h"
#include "server/connection/connection.h"

namespace redis_simple {
class Server {
 public:
  static Server* Get();
  void Run(const std::string& ip, const int& port);
  std::weak_ptr<ae::AeEventLoop> EventLoop() { return el_; }
  std::weak_ptr<const db::RedisDb> DB() { return db_; }
  void AddClient(Client* c) { clients_.push_back(c); }
  const std::vector<Client*>& Clients() { return clients_; }
  ~Server() {
    for (const Client* c : clients_) c->Free();
  }

 private:
  Server();
  void AcceptConnHandler();
  int ServerCron(long long id, std::any clientData);
  int fd_;
  int flags_;
  static std::shared_ptr<ae::AeEventLoop> el_;
  std::vector<Client*> clients_;
  std::shared_ptr<db::RedisDb> db_;
};
}  // namespace redis_simple
