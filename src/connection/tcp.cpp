#include "tcp.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <string>

namespace redis_simple {
namespace tcp {
namespace {
int tcpGenericConnect(int socket_fd, const sockaddr* addr, socklen_t len) {
  return connect(socket_fd, addr, len) < 0 ? TCPStatusCode::tcpError
                                           : TCPStatusCode::tcpOK;
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
  int s;
  if ((s = socket(domain, SOCK_STREAM, 0)) == -1) {
    return TCPStatusCode::tcpError;
  }
  if (tcpSetReuseAddr(s) == -1) {
    close(s);
    return TCPStatusCode::tcpError;
  }
  if (non_block) {
    if (nonBlock(s) == TCPStatusCode::tcpError) {
      close(s);
      return TCPStatusCode::tcpError;
    }
  }
  if (setCLOEXEC(s) == TCPStatusCode::tcpError) {
    close(s);
    return TCPStatusCode::tcpError;
  }
  return s;
}

int tcpConnect(int socket_fd, sockaddr* source_addr, socklen_t source_len,
               sockaddr* remote_addr, socklen_t remote_len) {
  if (remote_addr == nullptr) {
    return TCPStatusCode::tcpError;
  }
  if (source_addr && bind(socket_fd, source_addr, source_len) < 0) {
    return TCPStatusCode::tcpError;
  }
  if (tcpGenericConnect(socket_fd, remote_addr, remote_len) == -1) {
    if (!isNonBlock(socket_fd) || errno != EINPROGRESS) {
      perror("conn error: ");
      printf("connect fail with errno: %d\n", errno);

      close(socket_fd);
      return TCPStatusCode::tcpError;
    }

    printf("connect in progress %d\n", socket_fd);
  }
  return socket_fd;
}

int tcpAccept(int socket_fd, std::string* const ip, int* const port) {
  sockaddr_storage sa;
  socklen_t len = sizeof(sa);
  int remote_fd =
      tcpGenericAccept(socket_fd, reinterpret_cast<sockaddr*>(&sa), &len);
  if (remote_fd < 0) {
    if (!isNonBlock(socket_fd) || (errno != EWOULDBLOCK && errno != EAGAIN)) {
      close(socket_fd);
      return TCPStatusCode::tcpError;
    }
    return socket_fd;
  }
  struct sockaddr_in* s = reinterpret_cast<sockaddr_in*>(&sa);
  *ip = inet_ntoa(s->sin_addr);
  if (port) {
    *port = ntohs(s->sin_port);
  }
  return remote_fd;
}

int tcpListen(int socket_fd, struct sockaddr* addr, socklen_t len,
              int backlog) {
  if (bind(socket_fd, addr, len) < 0) {
    close(socket_fd);
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

int nonBlock(int socket_fd) { return setBlock(socket_fd, false); }

int block(int socket_fd) { return setBlock(socket_fd, true); }

bool isSocketError(int fd) {
  int sockerr = 0;
  socklen_t errlen;
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &sockerr, &errlen);
  printf("sock err %d\n", sockerr);
  return sockerr;
}
}  // namespace tcp
}  // namespace redis_simple
