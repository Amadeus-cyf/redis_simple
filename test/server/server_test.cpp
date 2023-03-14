#include "server/server.h"

#include "server/event_loop/ae.h"

namespace redis_simple {
void run() { Server::get()->run("localhost", 8081); }
}  // namespace redis_simple

int main() { redis_simple::run(); }
