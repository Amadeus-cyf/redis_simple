#include "tcp.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

namespace redis_simple {
namespace tcp {
namespace {
// Keep the listen queue small for this single-threaded test server.
constexpr const int kBacklog = 3;

int TcpSetReuseAddr(int socket_fd) {
  int yes = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ==
      -1) {
    return TcpStatusCode::kTcpError;
  }
  return kTcpOk;
}

int TcpGenericCreateSocket(int domain, int type, int protocol, bool non_block) {
  // Centralize socket setup so callers do not forget reuseaddr or nonblocking
  // mode when creating listening and client sockets.
  int socket_fd = socket(domain, type, protocol);
  if (socket_fd < 0) {
    return TcpStatusCode::kTcpError;
  }
  if (TcpSetReuseAddr(socket_fd) < 0) {
    close(socket_fd);
    return TcpStatusCode::kTcpError;
  }
  if (non_block && NonBlock(socket_fd) == TcpStatusCode::kTcpError) {
    close(socket_fd);
    return TcpStatusCode::kTcpError;
  }
  return socket_fd;
}

int TcpGenericAccept(int socket_fd, sockaddr* addr, socklen_t* len) {
  int s = -1;
  if ((s = accept(socket_fd, addr, len)) == -1) {
    return TcpStatusCode::kTcpError;
  }
  return s;
}

int SetBlock(int fd, bool block) {
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0) {
    return TcpStatusCode::kTcpError;
  }
  const bool is_non_block = (flags & O_NONBLOCK) != 0;
  if (is_non_block == !block) {
    return TcpStatusCode::kTcpOk;
  }
  block ? flags &= ~O_NONBLOCK : flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags) < 0 ? TcpStatusCode::kTcpError
                                       : TcpStatusCode::kTcpOk;
}

bool IsNonBlock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  return flags & O_NONBLOCK;
}

int SetCloseOnExec(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags & O_CLOEXEC) {
    return kTcpOk;
  }
  flags |= O_CLOEXEC;
  return fcntl(fd, F_SETFL, flags) < 0 ? TcpStatusCode::kTcpError
                                       : TcpStatusCode::kTcpOk;
}
}  // namespace

int TcpCreateSocket(int domain, bool non_block) {
  return TcpGenericCreateSocket(domain, SOCK_STREAM, 0, non_block);
}

int TcpBindAndConnect(const TcpAddrInfo& remote,
                      const std::optional<TcpAddrInfo>& local,
                      const bool non_block) {
  struct addrinfo hints, *info;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(remote.ip.c_str(), std::to_string(remote.port).c_str(),
                  &hints, &info) < 0) {
    return TcpStatusCode::kTcpError;
  }
  int socket_fd = -1;
  for (const addrinfo* p = info; p != nullptr; p = p->ai_next) {
    socket_fd = TcpGenericCreateSocket(p->ai_family, p->ai_socktype,
                                       p->ai_protocol, non_block);
    if (socket_fd < 0) {
      continue;
    }
    if (local.has_value() && TcpBind(socket_fd, local.value()) < 0) {
      RS_LOG_DEBUG("bind error: %s\n", std::strerror(errno));
      close(socket_fd);
      socket_fd = -1;
      continue;
    }
    if (connect(socket_fd, p->ai_addr, p->ai_addrlen) < 0) {
      // For nonblocking sockets, EINPROGRESS means the connection attempt is
      // still valid and completion will be reported by the writable event.
      if (!IsNonBlock(socket_fd) || errno != EINPROGRESS) {
        RS_LOG_DEBUG("connect error: %s\n", std::strerror(errno));
        close(socket_fd);
        socket_fd = -1;
        continue;
      }
      RS_LOG_DEBUG("connect in progress %d\n", socket_fd);
    }
    break;
  }
  freeaddrinfo(info);
  return socket_fd != -1 ? socket_fd : TcpStatusCode::kTcpError;
}

int TcpAccept(const int socket_fd, TcpAddrInfo* const addr_info) {
  sockaddr_storage sa;
  socklen_t len = sizeof(sa);
  int remote_fd = -1;
  // accept() may be interrupted by a signal; retry those transient failures.
  do {
    remote_fd =
        TcpGenericAccept(socket_fd, reinterpret_cast<sockaddr*>(&sa), &len);
  } while (remote_fd == -1 && errno == EINTR);
  if (NonBlock(remote_fd) == TcpStatusCode::kTcpError ||
      SetCloseOnExec(remote_fd) == TcpStatusCode::kTcpError) {
    close(remote_fd);
    return TcpStatusCode::kTcpError;
  }
  struct sockaddr_in* s = reinterpret_cast<sockaddr_in*>(&sa);
  if (addr_info) {
    addr_info->ip = inet_ntoa(s->sin_addr);
    addr_info->port = ntohs(s->sin_port);
  }
  return remote_fd;
}

int TcpBind(const int socket_fd, const TcpAddrInfo& addr_info) {
  struct addrinfo hints, *info = nullptr;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(addr_info.ip.c_str(), std::to_string(addr_info.port).c_str(),
                  &hints, &info) < 0) {
    return TcpStatusCode::kTcpError;
  }
  int r = TcpStatusCode::kTcpError;
  for (const addrinfo* p = info; p != nullptr; p = p->ai_next) {
    if (bind(socket_fd, p->ai_addr, p->ai_addrlen) < 0) {
      continue;
    }
    RS_LOG_DEBUG("bind success\n");
    r = TcpStatusCode::kTcpOk;
    break;
  }
  freeaddrinfo(info);
  return r;
}

int TcpListen(const int socket_fd) {
  if (listen(socket_fd, kBacklog) < 0) {
    close(socket_fd);
    return TcpStatusCode::kTcpError;
  }
  return TcpStatusCode::kTcpOk;
}

int NonBlock(const int socket_fd) { return SetBlock(socket_fd, false); }

int Block(const int socket_fd) { return SetBlock(socket_fd, true); }

bool IsSocketError(const int fd) {
  int socket_error = 0;
  socklen_t error_length = sizeof(socket_error);
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &error_length);
  RS_LOG_DEBUG("sock err %d\n", socket_error);
  return socket_error;
}
}  // namespace tcp
}  // namespace redis_simple
