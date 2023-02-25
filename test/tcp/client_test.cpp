#include <arpa/inet.h>
#include <stdlib.h>

#include "src/connection/tcp.h"

namespace redis_simple {
void run() {
  const std::string& ip = "localhost";

  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(8080);

  struct sockaddr_in remote;
  remote.sin_family = AF_INET;
  remote.sin_addr.s_addr = INADDR_ANY;
  remote.sin_port = htons(8081);

  int s = tcp::tcpCreateSocket(AF_INET, false);

  tcp::tcpConnect(s, (struct sockaddr*)&addr, sizeof(addr),
                  (struct sockaddr*)&remote, sizeof(remote));
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
