#include <arpa/inet.h>
#include <stdlib.h>

#include <optional>

#include "tcp/tcp.h"

namespace redis_simple {
void Run() {
  const tcp::TCPAddrInfo remote("localhost", 8080);
  const std::optional<const tcp::TCPAddrInfo>& local =
      std::make_optional<const tcp::TCPAddrInfo>("localhost", 8081);
  tcp::TCP_BindAndConnect(remote, local, false);
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
