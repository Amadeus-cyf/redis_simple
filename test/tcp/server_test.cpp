#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "tcp/tcp.h"

namespace redis_simple {
void run() {
  int s = tcp::tcpCreateSocket(AF_INET, false);
  printf("listen result: %d\n", tcp::tcpBindAndListen(s, "localhost", 8081));

  struct sockaddr_in remote;
  socklen_t len = sizeof(remote);

  std::string remote_ip;
  int a = 1;
  int* remote_port = &a;

  printf("accept result: %d\n", tcp::tcpAccept(s, &remote_ip, remote_port));

  printf("accept connection from %s:%d\n", remote_ip.c_str(), *remote_port);
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
