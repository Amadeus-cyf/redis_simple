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

struct TCPAddrInfo {
  std::string ip;
  int port;
  TCPAddrInfo() : ip(""), port(-1){};
  TCPAddrInfo(const std::string& ip, int port) : ip(ip), port(port){};
};

int TCP_CreateSocket(int domain, bool non_block);
int TCP_Connect(const TCPAddrInfo& remote,
                const std::optional<const TCPAddrInfo>& local,
                const bool non_block = false);
int TCP_Bind(const int socket_fd, const TCPAddrInfo& addrInfo);
int TCP_Listen(const int socket_fd);
int TCP_Accept(const int socket_fd, TCPAddrInfo* const addrInfo);
int Block(const int fd);
int NonBlock(const int fd);
bool IsSocketError(const int fd);
};  // namespace tcp
}  // namespace redis_simple
