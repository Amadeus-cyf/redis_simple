#include "tcp.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <string>

namespace redis_simple {
namespace tcp {
namespace {
int tcpGenericCreateSocket(int domain, int type, int protocol, bool non_block) {
  int socket_fd = socket(domain, type, protocol);
  if (socket_fd < 0) {
    return TCPStatusCode::tcpError;
  }
  if (tcpSetReuseAddr(socket_fd) < 0) {
    close(socket_fd);
    return TCPStatusCode::tcpError;
  }
  if (non_block && nonBlock(socket_fd) == TCPStatusCode::tcpError) {
    close(socket_fd);
    return TCPStatusCode::tcpError;
  }
  return socket_fd;
}

int tcpGenericAccept(int socket_fd, sockaddr* addr, socklen_t* len) {
  int s = -1;
  if ((s = accept(socket_fd, addr, len)) == -1) {
    return TCPStatusCode::tcpError;
  }
  return s;
}

int setBlock(int fd, bool block) {
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0) {
    return TCPStatusCode::tcpError;
  }
  if ((flags & O_NONBLOCK) == !block) {
    return tcpOK;
  }
  block ? flags &= ~O_NONBLOCK : flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags) < 0 ? TCPStatusCode::tcpError
                                       : TCPStatusCode::tcpOK;
}

bool isNonBlock(int fd) {
  int flags = fcntl(fd, F_GETFL);

  return flags & O_NONBLOCK;
}

int setCLOEXEC(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags & O_CLOEXEC) {
    return tcpOK;
  }
  flags |= O_CLOEXEC;
  return fcntl(fd, F_SETFL, flags) < 0 ? TCPStatusCode::tcpError
                                       : TCPStatusCode::tcpOK;
}
}  // namespace

int tcpCreateSocket(int domain, bool non_block) {
  return tcpGenericCreateSocket(domain, SOCK_STREAM, 0, non_block);
}

int tcpConnect(const std::string& remote_ip, const int remote_port,
               const bool non_block, const std::optional<std::string>& local_ip,
               const std::optional<int>& local_port) {
  struct addrinfo hints, *info;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(remote_ip.c_str(), std::to_string(remote_port).c_str(),
                  &hints, &info) < 0) {
    return TCPStatusCode::tcpError;
  }
  int socket_fd = -1;
  for (const addrinfo* p = info; p != nullptr; p = p->ai_next) {
    socket_fd = tcpGenericCreateSocket(p->ai_family, p->ai_socktype,
                                       p->ai_protocol, non_block);
    if (socket_fd < 0) {
      continue;
    }
    if (local_ip.has_value() && local_port.has_value() &&
        tcpBind(socket_fd, local_ip.value(), local_port.value()) ==
            TCPStatusCode::tcpError) {
      close(socket_fd);
      socket_fd = -1;
      continue;
    }
    if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
      if (!isNonBlock(socket_fd) || errno != EINPROGRESS) {
        perror("conn error");
        close(socket_fd);
        socket_fd = -1;
        continue;
      }
      printf("connect in progress %d\n", socket_fd);
    }
    break;
  }
  freeaddrinfo(info);
  return socket_fd != -1 ? socket_fd : TCPStatusCode::tcpError;
}

int tcpBind(const int socket_fd, const std::string& ip, const int port) {
  struct addrinfo hints, *info = nullptr;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &info) <
      0) {
    return TCPStatusCode::tcpError;
  }
  int r = TCPStatusCode::tcpError;
  for (const addrinfo* p = info; p != nullptr; p = p->ai_next) {
    if (bind(socket_fd, p->ai_addr, p->ai_addrlen) < 0) {
      continue;
    }
    printf("bind success\n");
    r = TCPStatusCode::tcpOK;
    break;
  }
  freeaddrinfo(info);
  return r;
}

int tcpAccept(const int socket_fd, std::string* const ip, int* const port) {
  sockaddr_storage sa;
  socklen_t len = sizeof(sa);
  int remote_fd = -1;
  do {
    remote_fd =
        tcpGenericAccept(socket_fd, reinterpret_cast<sockaddr*>(&sa), &len);
  } while (remote_fd == -1 && errno == EINTR);
  if (nonBlock(remote_fd) == TCPStatusCode::tcpError ||
      setCLOEXEC(remote_fd) == TCPStatusCode::tcpError) {
    close(remote_fd);
    return TCPStatusCode::tcpError;
  }
  struct sockaddr_in* s = reinterpret_cast<sockaddr_in*>(&sa);
  *ip = inet_ntoa(s->sin_addr);
  if (port) {
    *port = ntohs(s->sin_port);
  }
  return remote_fd;
}

int tcpListen(const int socket_fd, const std::string& ip, const int port) {
  if (tcpBind(socket_fd, ip, port) < 0) {
    return TCPStatusCode::tcpError;
  }
  if (listen(socket_fd, backlog) < 0) {
    close(socket_fd);
    return TCPStatusCode::tcpError;
  }
  return TCPStatusCode::tcpOK;
}

int tcpSetReuseAddr(int socket_fd) {
  int yes = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ==
      -1) {
    return TCPStatusCode::tcpError;
  }
  return tcpOK;
}

int nonBlock(const int socket_fd) { return setBlock(socket_fd, false); }

int block(const int socket_fd) { return setBlock(socket_fd, true); }

bool isSocketError(const int fd) {
  int sockerr = 0;
  socklen_t errlen;
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &sockerr, &errlen);
  printf("sock err %d\n", sockerr);
  return sockerr;
}
}  // namespace tcp
}  // namespace redis_simple
