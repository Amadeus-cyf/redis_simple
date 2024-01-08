#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "event_loop/ae.h"
#include "event_loop/ae_file_event_impl.h"
#include "tcp/tcp.h"

namespace redis_simple {
ae::AeEventStatus ReadProc(ae::AeEventLoop* el, int fd, int* client_data,
                           int mask) {
  printf("read data from fd %d\n", fd);
  std::string res;
  char buffer[1024];
  ssize_t r = 0;
  while ((r = read(fd, buffer, 1024)) != EOF) {
    res.append(buffer, r);
  }
  printf("receive resp after newline: %s\n", res.c_str());
  return ae::AeEventStatus::aeEventOK;
}

ae::AeEventStatus AcceptProc(ae::AeEventLoop* el, int fd, int* client_data,
                             int mask) {
  tcp::TCPAddrInfo addr;
  int port = 0;
  int remote_fd = tcp::TCP_Accept(fd, &addr);
  if (remote_fd < 0) {
    printf("accept failed\n");
    return ae::AeEventStatus::aeEventErr;
  }
  printf("accept %s:%d\n", addr.ip.c_str(), addr.port);
  ae::AeFileEvent* fe = ae::AeFileEventImpl<int>::Create(
      ReadProc, nullptr, client_data, ae::AeFlags::aeReadable);
  el->AeCreateFileEvent(remote_fd, fe);
  return ae::AeEventStatus::aeEventOK;
}

void run() {
  std::shared_ptr<ae::AeEventLoop> el(ae::AeEventLoop::InitEventLoop());
  int fd = tcp::TCP_CreateSocket(AF_INET, true);
  if (fd < 0) {
    printf("failed to create socket\n");
    return;
  }
  printf("create socket fd %d\n", fd);
  const tcp::TCPAddrInfo addr("localhost", 8080);
  if (tcp::TCP_Bind(fd, addr) == tcp::TCPStatusCode::tcpError) {
    printf("failed to listen to %s:%d", "localhost", 8080);
    return;
  }
  if (tcp::TCP_Listen(fd) == tcp::TCPStatusCode::tcpError) {
    printf("failed to listen to %s:%d", "localhost", 8080);
    return;
  }
  int client_data = 10000;
  ae::AeFileEvent* fe = ae::AeFileEventImpl<int>::Create(
      AcceptProc, nullptr, &client_data, ae::AeFlags::aeReadable);
  el->AeCreateFileEvent(fd, fe);
  el->AeMain();
  return;
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
