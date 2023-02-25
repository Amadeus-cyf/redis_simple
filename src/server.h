#pragma once

#include <string>
#include <vector>

#include "src/client.h"
#include "src/connection/connection.h"
#include "src/event_loop/ae.h"

namespace redis_simple {
class Server {
 public:
  static Server* get();
  void run(const std::string& ip, const int& port);
  void acceptConnHandler();
  void bindEventLoop(ae::AeEventLoop* el) { this->el = el; }
  ae::AeEventLoop* getEventLoop() { return el; }
  db::RedisDb* getDb() { return db.get(); }
  void addClient(Client* c) { clients.push_back(c); }
  const std::vector<Client*>& getClients() { return clients; }
  int serverCron(long long id, void* clientData);

 private:
  Server();
  int fd;
  ae::AeEventLoop* el;
  int flags;
  std::vector<Client*> clients;
  std::unique_ptr<db::RedisDb> db;
};
}  // namespace redis_simple
