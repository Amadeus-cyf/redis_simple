#pragma once

#include <sys/socket.h>

#include <optional>
#include <string>

namespace redis_simple {
namespace tcp {
enum TcpStatusCode {
  kTcpError = -1,
  kTcpOk = 0,
};

struct TcpAddrInfo {
  std::string ip;
  int port;
  TcpAddrInfo() : ip(""), port(-1){};
  TcpAddrInfo(const std::string& ip, int port) : ip(ip), port(port){};
};

int TcpCreateSocket(int domain, bool non_block);
int TcpBindAndConnect(const TcpAddrInfo& remote,
                      const std::optional<TcpAddrInfo>& local,
                      const bool non_block = true);
int TcpBind(const int socket_fd, const TcpAddrInfo& addr_info);
int TcpListen(const int socket_fd);
int TcpAccept(const int socket_fd, TcpAddrInfo* const addr_info);
int Block(const int fd);
int NonBlock(const int fd);
bool IsSocketError(const int fd);
};  // namespace tcp
}  // namespace redis_simple
