#pragma once

#include <sys/socket.h>

#include <optional>
#include <string>

namespace redis_simple::tcp {
enum class TcpStatusCode {
  kTcpError = -1,
  kTcpOk = 0,
};

constexpr int ToInt(TcpStatusCode status) { return static_cast<int>(status); }
constexpr bool operator==(int value, TcpStatusCode status) {
  return value == ToInt(status);
}
constexpr bool operator!=(int value, TcpStatusCode status) {
  return value != ToInt(status);
}
constexpr bool operator==(TcpStatusCode status, int value) {
  return value == status;
}
constexpr bool operator!=(TcpStatusCode status, int value) {
  return value != status;
}

struct TcpAddrInfo {
  std::string ip;
  int port;
  TcpAddrInfo() : ip(""), port(-1) {}
  TcpAddrInfo(const std::string& ip, int port) : ip(ip), port(port) {}
};

int TcpCreateSocket(int domain, bool non_block);
int TcpBindAndConnect(const TcpAddrInfo& remote,
                      const std::optional<TcpAddrInfo>& local,
                      bool non_block = true);
int TcpBind(int socket_fd, const TcpAddrInfo& addr_info);
int TcpListen(int socket_fd);
int TcpAccept(int socket_fd, TcpAddrInfo* const addr_info);
int Block(int fd);
int NonBlock(int fd);
bool IsSocketError(int fd);
}  // namespace redis_simple::tcp
