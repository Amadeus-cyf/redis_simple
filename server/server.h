#pragma once

#include <string>
#include <vector>

#include "event_loop/ae.h"
#include "server/client.h"
#include "server/connection/connection.h"

namespace redis_simple {
class Server {
 public:
  static Server* get();
  void run(const std::string& ip, const int& port);
  ae::AeEventLoop* getEventLoop() { return el.get(); }
  db::RedisDb* getDb() { return db.get(); }
  void addClient(Client* c) { clients.push_back(c); }
  const std::vector<Client*>& getClients() { return clients; }

 private:
  Server();
  void acceptConnHandler();
  int serverCron(long long id, void* clientData);
  int fd;
  static std::unique_ptr<ae::AeEventLoop> el;
  int flags;
  std::vector<Client*> clients;
  std::unique_ptr<db::RedisDb> db;
};
}  // namespace redis_simple
