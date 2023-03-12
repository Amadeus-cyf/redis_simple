#pragma once

#include <sys/socket.h>

#include <optional>
#include <string>

namespace redis_simple {
enum TCPStatusCode {
  tcpError = -1,
  tcpOK = 0,
};

namespace tcp {
static constexpr const int backlog = 3;
int tcpCreateSocket(int domain, bool non_block);
int tcpConnect(const std::string& remote_ip, const int remote_port,
               const bool non_block = false,
               const std::optional<std::string>& local_ip = std::nullopt,
               const std::optional<int>& local_port = std::nullopt);
int tcpBind(const int socket_fd, const std::string& ip, const int port);
int tcpListen(const int socket_fd, const std::string& ip, const int port);
int tcpAccept(const int socket_fd, std::string* const ip, int* const port);
int tcpSetReuseAddr(const int fd);
int block(const int fd);
int nonBlock(const int fd);
bool isSocketError(const int fd);
};  // namespace tcp
}  // namespace redis_simple
