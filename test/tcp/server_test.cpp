#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "tcp/tcp.h"

namespace redis_simple {
void Run() {
  int s = tcp::TCP_CreateSocket(AF_INET, false);
  const tcp::TCPAddrInfo local("localhost", 8080);
  printf("bind result: %d\n", tcp::TCP_Bind(s, local));
  printf("listen result: %d\n", tcp::TCP_Listen(s));

  struct sockaddr_in remote;
  socklen_t len = sizeof(remote);

  tcp::TCPAddrInfo remote_addr;
  printf("accept result: %d\n", tcp::TCP_Accept(s, &remote_addr));
  printf("accept connection from %s:%d\n", remote_addr.ip.c_str(),
         remote_addr.port);
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
