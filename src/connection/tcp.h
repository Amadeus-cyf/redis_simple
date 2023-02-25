#pragma once

#include <sys/socket.h>

#include <string>

namespace redis_simple {
enum TCPStatusCode {
  tcpError = -1,
  tcpOK = 0,
};

namespace tcp {
int tcpCreateSocket(int domain, bool non_block);
int tcpConnect(int socket_fd, sockaddr* source_addr, socklen_t source_len,
               sockaddr* remote_addr, socklen_t remote_len);
int tcpListen(int socket_fd, struct sockaddr* addr, socklen_t len, int backlog);
int tcpAccept(int socket_fd, std::string* ip, int* port);
int tcpSetReuseAddr(int fd);
int block(int fd);
int nonBlock(int fd);
bool isSocketError(int fd);
};  // namespace tcp
}  // namespace redis_simple
