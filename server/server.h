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
  std::weak_ptr<const ae::AeEventLoop> EventLoop() { return el; }
  std::weak_ptr<const db::RedisDb> DB() { return db; }
  void AddClient(Client* c) { clients.push_back(c); }
  const std::vector<Client*>& Clients() { return clients; }
  ~Server() {
    for (const Client* c : clients) c->free();
  }

 private:
  Server();
  void AcceptConnHandler();
  int ServerCron(long long id, void* clientData);
  int fd;
  static std::shared_ptr<ae::AeEventLoop> el;
  int flags;
  std::vector<Client*> clients;
  std::shared_ptr<db::RedisDb> db;
};
}  // namespace redis_simple
