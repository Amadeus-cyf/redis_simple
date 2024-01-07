#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "tcp/tcp.h"

namespace redis_simple {
void Run() {
  int s = tcp::TCP_CreateSocket(AF_INET, false);
  printf("bind result: %d\n", tcp::TCP_Bind(s, "localhost", 8080));
  printf("listen result: %d\n", tcp::TCP_Listen(s, "localhost", 8080));

  struct sockaddr_in remote;
  socklen_t len = sizeof(remote);

  std::string remote_ip;
  int a = 1;
  int* remote_port = &a;

  printf("accept result: %d\n", tcp::TCP_Accept(s, &remote_ip, remote_port));
  printf("accept connection from %s:%d\n", remote_ip.c_str(), *remote_port);
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
