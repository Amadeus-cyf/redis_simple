#include "server/server.h"

#include "event_loop/ae.h"

namespace redis_simple {
void Run() { Server::Get()->Run("localhost", 8080); }
}  // namespace redis_simple

int main() { redis_simple::Run(); }
