#pragma once

#include <sys/socket.h>

#include <optional>
#include <string>

namespace redis_simple {
namespace tcp {
enum TCPStatusCode {
  tcpError = -1,
  tcpOK = 0,
};
int TCP_CreateSocket(int domain, bool non_block);
int TCP_Connect(const std::string& remote_ip, const int remote_port,
                const bool non_block = false,
                const std::optional<std::string>& local_ip = std::nullopt,
                const std::optional<int>& local_port = std::nullopt);
int TCP_Bind(const int socket_fd, const std::string& ip, const int port);
int TCP_Listen(const int socket_fd, const std::string& ip, const int port);
int TCP_Accept(const int socket_fd, std::string* const ip, int* const port);
int Block(const int fd);
int NonBlock(const int fd);
bool IsSocketError(const int fd);
};  // namespace tcp
}  // namespace redis_simple
