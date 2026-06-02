#include <arpa/inet.h>
#include <stdlib.h>

#include <optional>

#include "tcp/tcp.h"

namespace redis_simple {
void Run() {
  const tcp::TCPAddrInfo remote("localhost", 8080);
  const auto local = std::make_optional<tcp::TCPAddrInfo>("localhost", 8081);
  tcp::TCP_BindAndConnect(remote, local, false);
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
