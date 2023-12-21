#include <arpa/inet.h>
#include <stdlib.h>

#include "tcp/tcp.h"

namespace redis_simple {
void Run() { tcp::TCP_Connect("localhost", 8081, false, "localhost", 8080); }
}  // namespace redis_simple

int main() { redis_simple::Run(); }
