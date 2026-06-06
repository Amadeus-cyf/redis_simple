#include <arpa/inet.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <optional>
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
  const tcp::TcpAddrInfo remote("localhost", 8080);
  const auto local = std::make_optional<tcp::TcpAddrInfo>("localhost", 8081);
  const int fd = tcp::TcpBindAndConnect(remote, local, false);
  if (fd < 0) {
    RS_LOG_DEBUG("failed to connect to tcp integration server\n");
    return EXIT_FAILURE;
  }
  if (write(fd, kRequest, strlen(kRequest)) != strlen(kRequest)) {
    RS_LOG_DEBUG("failed to write tcp integration request\n");
    close(fd);
    return EXIT_FAILURE;
  }
  char buffer[16] = {};
  const ssize_t nread = ReadWithTimeout(fd, buffer, sizeof(buffer));
  close(fd);
  if (nread != strlen(kResponse) || std::string(buffer, nread) != kResponse) {
    RS_LOG_DEBUG("unexpected tcp response: %s\n", buffer);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
}  // namespace redis_simple

int main() { return redis_simple::Run(); }
