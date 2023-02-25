#include "server.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include "db/db.h"
#include "event_loop/ae_file_event.h"
#include "expire.h"
#include "networking/networking.h"

namespace redis_simple {
Server::Server() : db(db::RedisDb::initDb()) {}

Server* Server::get() {
  static std::unique_ptr<Server> server;
  if (!server) {
    server = std::unique_ptr<Server>(new Server());
  }
  return server.get();
}

void Server::run(const std::string& ip, const int& port) {
  connection::Connection conn;
  if (conn.listen(ip, port) == connection::StatusCode::c_err) {
    return;
  }
  fd = conn.getFd();
  acceptConnHandler();
  ae::aeTimeProc time_proc = [](long long id, void* clientData) {
    return get()->serverCron(id, clientData);
  };
  el->aeCreateTimeEvent(
      ae::AeTimeEvent::createAeTimeEvent(time_proc, nullptr, nullptr));
}

void Server::acceptConnHandler() {
  ae::AeFileEvent* fe = ae::AeFileEvent::create(networking::acceptHandler,
                                                nullptr, this, ae::aeReadable);
  if (el->aeCreateFileEvent(fd, fe) < 0) {
    printf("error in adding client creation file event");
  }
}

int Server::serverCron(long long id, void* clientData) {
  activeExpireCycle();
  return 1;
}
}  // namespace redis_simple

int main() {
  redis_simple::ae::AeEventLoop* el =
      redis_simple::ae::AeEventLoop::initEventLoop();
  redis_simple::Server::get()->bindEventLoop(el);
  redis_simple::Server::get()->run("localhost", 8081);

  el->aeMain();
}
