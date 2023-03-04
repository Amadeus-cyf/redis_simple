#include "server/server.h"

#include "server/event_loop/ae.h"

namespace redis_simple {
void run() {
  ae::AeEventLoop* el = ae::AeEventLoop::initEventLoop();
  Server::get()->bindEventLoop(el);
  Server::get()->run("localhost", 8081);
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
