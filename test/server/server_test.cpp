#include "src/server.h"

#include "src/event_loop/ae.h"

namespace redis_simple {
void run() {
  ae::AeEventLoop* el = ae::AeEventLoop::initEventLoop();
  Server::get()->bindEventLoop(el);
  Server::get()->run("localhost", 8081);

  el->aeMain();
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
