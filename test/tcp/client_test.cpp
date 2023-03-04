#include <arpa/inet.h>
#include <stdlib.h>

#include "server/connection/tcp.h"

namespace redis_simple {
void run() { tcp::tcpConnect("localhost", 8081, "localhost", 8080); }
}  // namespace redis_simple

int main() { redis_simple::run(); }
