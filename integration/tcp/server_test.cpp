#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include "tcp/tcp.h"

namespace redis_simple {
namespace {
constexpr std::string_view kRequest = "ping";
constexpr std::string_view kResponse = "pong";

ssize_t ReadWithTimeout(int fd, char* buffer, size_t len) {
  pollfd pfd{};
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
  int s = tcp::TcpCreateSocket(AF_INET, false);
  if (s < 0) {
    RS_LOG_DEBUG("failed to create socket\n");
    return EXIT_FAILURE;
  }
  const tcp::TcpAddrInfo local("localhost", 8080);
  if (tcp::TcpBind(s, local) != tcp::TcpStatusCode::kOk) {
    RS_LOG_DEBUG("failed to bind tcp integration server\n");
    close(s);
    return EXIT_FAILURE;
  }
  if (tcp::TcpListen(s) != tcp::TcpStatusCode::kOk) {
    RS_LOG_DEBUG("failed to listen on tcp integration server\n");
    close(s);
    return EXIT_FAILURE;
  }

  tcp::TcpAddrInfo remote_addr;
  const int remote_fd = tcp::TcpAccept(s, &remote_addr);
  if (remote_fd < 0) {
    RS_LOG_DEBUG("failed to accept tcp integration client\n");
    close(s);
    return EXIT_FAILURE;
  }
  RS_LOG_DEBUG("accept connection from %s:%d\n", remote_addr.ip.c_str(),
               remote_addr.port);

  std::array<char, 16> buffer{};
  const ssize_t nread =
      ReadWithTimeout(remote_fd, buffer.data(), buffer.size());
  if (nread != static_cast<ssize_t>(kRequest.size()) ||
      std::string_view(buffer.data(), nread) != kRequest) {
    RS_LOG_DEBUG("unexpected tcp request: %s\n", buffer.data());
    close(remote_fd);
    close(s);
    return EXIT_FAILURE;
  }
  if (write(remote_fd, kResponse.data(), kResponse.size()) !=
      static_cast<ssize_t>(kResponse.size())) {
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

int main() {
  try {
    return redis_simple::Run();
  } catch (...) {
    return EXIT_FAILURE;
  }
}
