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
static constexpr const int backlog = 3;

static int TCP_Bind(const int socket_fd, const std::string& ip,
                    const int port) {
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

static int TCP_SetReuseAddr(int socket_fd) {
  int yes = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ==
      -1) {
    return TCPStatusCode::tcpError;
  }
  return tcpOK;
}

static int TCP_GenericCreateSocket(int domain, int type, int protocol,
                                   bool non_block) {
  int socket_fd = socket(domain, type, protocol);
  if (socket_fd < 0) {
    return TCPStatusCode::tcpError;
  }
  if (TCP_SetReuseAddr(socket_fd) < 0) {
    close(socket_fd);
    return TCPStatusCode::tcpError;
  }
  if (non_block && NonBlock(socket_fd) == TCPStatusCode::tcpError) {
    close(socket_fd);
    return TCPStatusCode::tcpError;
  }
  return socket_fd;
}

static int TCP_GenericAccept(int socket_fd, sockaddr* addr, socklen_t* len) {
  int s = -1;
  if ((s = accept(socket_fd, addr, len)) == -1) {
    return TCPStatusCode::tcpError;
  }
  return s;
}

static int SetBlock(int fd, bool block) {
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0) {
    return TCPStatusCode::tcpError;
  }
  if ((flags & O_NONBLOCK) == !block) {
    return TCPStatusCode::tcpOK;
  }
  block ? flags &= ~O_NONBLOCK : flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags) < 0 ? TCPStatusCode::tcpError
                                       : TCPStatusCode::tcpOK;
}

static bool IsNonBlock(int fd) {
  int flags = fcntl(fd, F_GETFL);

  return flags & O_NONBLOCK;
}

static int SetCLOEXEC(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags & O_CLOEXEC) {
    return tcpOK;
  }
  flags |= O_CLOEXEC;
  return fcntl(fd, F_SETFL, flags) < 0 ? TCPStatusCode::tcpError
                                       : TCPStatusCode::tcpOK;
}
}  // namespace

int TCP_CreateSocket(int domain, bool non_block) {
  return TCP_GenericCreateSocket(domain, SOCK_STREAM, 0, non_block);
}

int TCP_Connect(const std::string& remote_ip, const int remote_port,
                const bool non_block,
                const std::optional<std::string>& local_ip,
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
    socket_fd = TCP_GenericCreateSocket(p->ai_family, p->ai_socktype,
                                        p->ai_protocol, non_block);
    if (socket_fd < 0) {
      continue;
    }
    if (local_ip.has_value() && local_port.has_value() &&
        TCP_Bind(socket_fd, local_ip.value(), local_port.value()) ==
            TCPStatusCode::tcpError) {
      close(socket_fd);
      socket_fd = -1;
      continue;
    }
    if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
      if (!IsNonBlock(socket_fd) || errno != EINPROGRESS) {
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

int TCP_Accept(const int socket_fd, std::string* const ip, int* const port) {
  sockaddr_storage sa;
  socklen_t len = sizeof(sa);
  int remote_fd = -1;
  do {
    remote_fd =
        TCP_GenericAccept(socket_fd, reinterpret_cast<sockaddr*>(&sa), &len);
  } while (remote_fd == -1 && errno == EINTR);
  if (NonBlock(remote_fd) == TCPStatusCode::tcpError ||
      SetCLOEXEC(remote_fd) == TCPStatusCode::tcpError) {
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

int TCP_BindAndListen(const int socket_fd, const std::string& ip,
                      const int port) {
  if (TCP_Bind(socket_fd, ip, port) < 0) {
    return TCPStatusCode::tcpError;
  }
  if (listen(socket_fd, backlog) < 0) {
    close(socket_fd);
    return TCPStatusCode::tcpError;
  }
  return TCPStatusCode::tcpOK;
}

int NonBlock(const int socket_fd) { return SetBlock(socket_fd, false); }

int Block(const int socket_fd) { return SetBlock(socket_fd, true); }

bool IsSocketError(const int fd) {
  int sockerr = 0;
  socklen_t errlen;
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &sockerr, &errlen);
  printf("sock err %d\n", sockerr);
  return sockerr;
}
}  // namespace tcp
}  // namespace redis_simple
