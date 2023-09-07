#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "event_loop/ae.h"
#include "event_loop/ae_file_event_impl.h"
#include "tcp/tcp.h"

namespace redis_simple {
ae::AeEventStatus readProc(ae::AeEventLoop* el, int fd, int* client_data,
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

ae::AeEventStatus acceptProc(ae::AeEventLoop* el, int fd, int* client_data,
                             int mask) {
  std::string ip;
  int port = 0;
  int remote_fd = tcp::tcpAccept(fd, &ip, &port);
  if (remote_fd < 0) {
    printf("accept failed\n");
    return ae::AeEventStatus::aeEventErr;
  }
  printf("accept %s:%d\n", ip.c_str(), port);
  ae::AeFileEvent* fe = ae::AeFileEventImpl<int>::create(
      readProc, nullptr, client_data, ae::AeFlags::aeReadable);
  el->aeCreateFileEvent(remote_fd, fe);
  return ae::AeEventStatus::aeEventOK;
}

void run() {
  std::unique_ptr<ae::AeEventLoop> el = ae::AeEventLoop::initEventLoop();
  int fd = tcp::tcpCreateSocket(AF_INET, true);
  if (fd < 0) {
    printf("failed to create socket\n");
    return;
  }
  printf("create socket fd %d\n", fd);
  if (tcp::tcpBindAndListen(fd, "localhost", 8081) ==
      tcp::TCPStatusCode::tcpError) {
    printf("failed to listen to %s:%d", "localhost", 8081);
    return;
  }
  int client_data = 10000;
  ae::AeFileEvent* fe = ae::AeFileEventImpl<int>::create(
      acceptProc, nullptr, &client_data, ae::AeFlags::aeReadable);
  el->aeCreateFileEvent(fd, fe);
  el->aeMain();
  return;
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
