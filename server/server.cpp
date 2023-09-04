#include "server.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include "db/db.h"
#include "event_loop/ae_file_event_impl.h"
#include "event_loop/ae_time_event_impl.h"
#include "expire.h"
#include "networking/networking.h"

namespace redis_simple {
std::unique_ptr<ae::AeEventLoop> Server::el = ae::AeEventLoop::initEventLoop();

Server::Server() : db(db::RedisDb::init()) {}

Server* Server::get() {
  static std::unique_ptr<Server> server;
  if (!server) {
    server = std::unique_ptr<Server>(new Server());
  }
  return server.get();
}

void Server::run(const std::string& ip, const int& port) {
  const connection::Context& ctx = {.fd = -1, .loop = el.get()};
  connection::Connection conn(ctx);
  if (conn.listen(ip, port) == connection::StatusCode::c_err) {
    return;
  }
  fd = conn.getFd();
  acceptConnHandler();
  ae::AeTimeEventImpl<Server>::aeTimeProc time_proc = [](long long id,
                                                         Server* clientData) {
    return get()->serverCron(id, clientData);
  };
  el->aeCreateTimeEvent(
      ae::AeTimeEventImpl<Server>::create(time_proc, nullptr, this));
  el->aeMain();
}

void Server::acceptConnHandler() {
  ae::AeFileEvent* fe = ae::AeFileEventImpl<Server>::create(
      networking::acceptHandler, nullptr, this, ae::aeReadable);
  if (el->aeCreateFileEvent(fd, fe) < 0) {
    printf("error in adding client creation file event");
  }
}

int Server::serverCron(long long id, void* clientData) {
  activeExpireCycle();
  return 1;
}
}  // namespace redis_simple
