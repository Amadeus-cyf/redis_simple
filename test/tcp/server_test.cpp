#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "src/connection/tcp.h"

namespace redis_simple {
void run() {
  int s = tcp::tcpCreateSocket(AF_INET, false);

  printf("create socket result: %d\n", s);

  const std::string& ip = "localhost";
  const int port = 8081;

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip != "localhost" ? inet_addr(ip.c_str()) : INADDR_ANY;
  addr.sin_port = htons(8081);

  printf("listen result: %d\n",
         tcp::tcpListen(s, (struct sockaddr*)&addr, sizeof(addr), 3));

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
