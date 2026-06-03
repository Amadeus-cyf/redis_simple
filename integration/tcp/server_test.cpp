#include <arpa/inet.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "tcp/tcp.h"

namespace redis_simple {
namespace {
constexpr const char* kRequest = "ping";
constexpr const char* kResponse = "pong";

ssize_t ReadWithTimeout(int fd, char* buffer, size_t len) {
  pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  if (poll(&pfd, 1, 1000) <= 0) {
    return -1;
  }
  return read(fd, buffer, len);
}
}  // namespace

int Run() {
  int s = tcp::TCP_CreateSocket(AF_INET, false);
  if (s < 0) {
    RS_LOG_DEBUG("failed to create socket\n");
    return EXIT_FAILURE;
  }
  const tcp::TCPAddrInfo local("localhost", 8080);
  if (tcp::TCP_Bind(s, local) != tcp::TCPStatusCode::tcpOK) {
    RS_LOG_DEBUG("failed to bind tcp integration server\n");
    close(s);
    return EXIT_FAILURE;
  }
  if (tcp::TCP_Listen(s) != tcp::TCPStatusCode::tcpOK) {
    RS_LOG_DEBUG("failed to listen on tcp integration server\n");
    close(s);
    return EXIT_FAILURE;
  }

  tcp::TCPAddrInfo remote_addr;
  const int remote_fd = tcp::TCP_Accept(s, &remote_addr);
  if (remote_fd < 0) {
    RS_LOG_DEBUG("failed to accept tcp integration client\n");
    close(s);
    return EXIT_FAILURE;
  }
  RS_LOG_DEBUG("accept connection from %s:%d\n", remote_addr.ip.c_str(),
               remote_addr.port);

  char buffer[16] = {};
  const ssize_t nread = ReadWithTimeout(remote_fd, buffer, sizeof(buffer));
  if (nread != strlen(kRequest) || std::string(buffer, nread) != kRequest) {
    RS_LOG_DEBUG("unexpected tcp request: %s\n", buffer);
    close(remote_fd);
    close(s);
    return EXIT_FAILURE;
  }
  if (write(remote_fd, kResponse, strlen(kResponse)) != strlen(kResponse)) {
    RS_LOG_DEBUG("failed to write tcp integration response\n");
    close(remote_fd);
    close(s);
    return EXIT_FAILURE;
  }
  close(remote_fd);
  close(s);
  return EXIT_SUCCESS;
}
}  // namespace redis_simple

int main() { return redis_simple::Run(); }
