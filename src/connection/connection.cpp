#include "connection.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

namespace redis_simple {
namespace connection {
Connection::Connection()
    : fd(-1),
      state(ConnState::connStateConnect),
      read_handler(nullptr),
      write_handler(nullptr),
      accept_handler(nullptr) {}

Connection::Connection(int cfd)
    : fd(cfd),
      state(ConnState::connStateConnect),
      read_handler(nullptr),
      write_handler(nullptr),
      accept_handler(nullptr) {}

StatusCode Connection::connect(const std::string& src_ip, int src_port,
                               const std::string& dest_ip, int dest_port) {
  if (!boundEventLoop()) {
    return StatusCode::c_err;
  }

  struct sockaddr_in src_addr;
  src_addr.sin_family = AF_INET;
  src_addr.sin_addr.s_addr =
      src_ip != "localhost" ? inet_addr(src_ip.c_str()) : INADDR_ANY;
  src_addr.sin_port = htons(src_port);

  struct sockaddr_in dest_addr;
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_addr.s_addr =
      dest_ip != "localhost" ? inet_addr(dest_ip.c_str()) : INADDR_ANY;
  dest_addr.sin_port = htons(dest_port);

  int s = tcp::tcpCreateSocket(AF_INET, true);
  if (s == -1) {
    return StatusCode::c_err;
  }
  s = tcp::tcpConnect(s, (struct sockaddr*)&src_addr, sizeof(src_addr),
                      (struct sockaddr*)&dest_addr, sizeof(dest_addr));
  if (s == -1) {
    return StatusCode::c_err;
  }
  setState(ConnState::connStateConnecting);
  printf("connect success %d\n", s);
  ae::AeFileEvent* e = ae::AeFileEvent::create(nullptr, connSocketEventHandler,
                                               this, ae::AeFlags::aeWritable);
  if (el->aeCreateFileEvent(s, e) < 0) {
    printf("adding connection socket event handler error");
    return StatusCode::c_err;
  }

  fd = s;
  return StatusCode::c_ok;
}

StatusCode Connection::listen(const std::string& ip, int port) {
  int s = tcp::tcpCreateSocket(AF_INET, true);

  if (s < 0) {
    printf("create socket failed\n");
    return StatusCode::c_err;
  }

  fd = s;
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr =
      ip != "localhost" ? inet_addr(ip.c_str()) : INADDR_ANY;
  address.sin_port = htons(port);
  return tcp::tcpListen(s, (struct sockaddr*)(&address), sizeof(address), 3) ==
                 TCPStatusCode::tcpOK
             ? StatusCode::c_ok
             : StatusCode::c_err;
}

StatusCode Connection::accept(std::string* dest_ip, int* dest_port) {
  if (fd < 0) {
    printf("socket not created\n");
    return StatusCode::c_err;
  }

  int s = tcp::tcpAccept(fd, dest_ip, dest_port);
  if (s < 0) {
    return StatusCode::c_err;
  }

  fd = s;
  tcp::nonBlock(fd);
  if (accept_handler) accept_handler->handle(this);
  if (state == ConnState::connStateAccepting) {
    state = ConnState::connStateConnected;
  } else {
    state = ConnState::connStateError;
  }
  printf("conn state accept %d\n", state);
  return StatusCode::c_ok;
}

void Connection::setReadHandler(std::unique_ptr<ConnHandler> rHandler) {
  if (!rHandler) {
    unsetReadHandler();
  } else {
    ae::AeFileEvent* e = ae::AeFileEvent::create(
        connSocketEventHandler, nullptr, this, ae::AeFlags::aeReadable);

    el->aeCreateFileEvent(fd, e);

    read_handler = move(rHandler);
    flags |= ae::AeFlags::aeReadable;
  }
}

void Connection::unsetReadHandler() {
  read_handler = nullptr;
  el->aeDelFileEvent(fd, ae::AeFlags::aeReadable);
  flags &= ~ae::AeFlags::aeReadable;
}

void Connection::setWriteHandler(std::unique_ptr<ConnHandler> wHandler) {
  if (!wHandler) {
    unsetWriteHandler();
  } else {
    ae::AeFileEvent* e = ae::AeFileEvent::create(
        nullptr, connSocketEventHandler, this, ae::AeFlags::aeWritable);

    el->aeCreateFileEvent(fd, e);
    write_handler = std::move(wHandler);
    flags |= ae::AeFlags::aeWritable;
  }
}

void Connection::unsetWriteHandler() {
  write_handler = nullptr;
  el->aeDelFileEvent(fd, ae::AeFlags::aeWritable);
  flags &= ~ae::AeFlags::aeWritable;
}

ae::AeEventStatus Connection::connSocketEventHandler(int fd, void* client_data,
                                                     int mask) {
  printf("event handler called with fd = %d, mask_read = %d, mask_write = %d\n",
         fd, mask & ae::AeFlags::aeReadable, mask & ae::AeFlags::aeWritable);

  Connection* conn = static_cast<Connection*>(client_data);
  if (conn == nullptr) {
    return ae::AeEventStatus::aeEventErr;
  }

  printf("event read handler %d\n", conn->read_handler.get() == nullptr);
  printf("state: %d\n", conn->getState());
  if (conn->getState() == ConnState::connStateConnecting &&
      (mask & ae::AeFlags::aeWritable)) {
    if (tcp::isSocketError(fd)) {
      printf("socker error\n");
      conn->setState(ConnState::connStateError);
      return ae::AeEventStatus::aeEventErr;
    } else {
      printf("connection state set to connected\n");
      conn->setState(ConnState::connStateConnected);
    }
  }

  int invert = conn->flags & ae::AeFlags::aeBarrier;
  if (!invert && (mask & ae::AeFlags::aeReadable) && conn->read_handler) {
    conn->read_handler->handle(conn);
  }
  if ((mask & ae::AeFlags::aeWritable) && conn->write_handler) {
    conn->write_handler->handle(conn);
  }
  if (invert && (mask & ae::AeFlags::aeReadable) && conn->read_handler) {
    conn->read_handler->handle(conn);
  }
  return ae::AeEventStatus::aeEventOK;
}

ssize_t Connection::connRead(const char* buffer, size_t readlen) {
  ssize_t nread = 0;
  int r = 0;
  while (nread < readlen &&
         (r = read(fd, (char*)buffer + nread, 1024)) != EOF) {
    if (r) {
      nread += r;
    } else {
      break;
    }
  }
  return nread;
}

ssize_t Connection::connRead(std::string& s) {
  char buffer[1024];
  int r = 0;
  int nread = 0;

  while ((r = read(fd, buffer + nread, 1024)) != EOF) {
    if (r == 0) {
      break;
    }
    s.append(buffer, r);
  }
  return s.length();
}

ssize_t Connection::connSyncRead(const char* buffer, size_t readlen,
                                 long timeout) {
  int r = el->aeWait(fd, ae::AeFlags::aeReadable, timeout);
  if (r < 0) {
    printf("conn sync read failed for connection %d\n", fd);
    return -1;
  } else if (r == 0) {
    printf("aeWait timeout\n");
    return 0;
  }

  return connRead(buffer, readlen);
}

ssize_t Connection::connSyncRead(std::string& s, long timeout) {
  int r = el->aeWait(fd, ae::AeFlags::aeReadable, timeout);

  if (r < 0) {
    printf("conn sync read string failed for connection %d\n", fd);
    return -1;
  } else if (r == 0) {
    printf("aeWait timeout\n");
    return 0;
  }

  printf("Conn start read sync\n");
  return connRead(s);
}

ssize_t Connection::connSyncReadline(std::string& s, long timeout) {
  int r = el->aeWait(fd, ae::AeFlags::aeReadable, timeout);
  if (r < 0) {
    printf("conn sync readline failed for connection %d\n", fd);
    return -1;
  } else if (r == 0) {
    printf("aeWait timeout\n");
    return 0;
  }

  r = 0;
  char buffer[1];

  while ((r = read(fd, buffer, 1)) != EOF) {
    if (r == 0) {
      break;
    }
    if (buffer[0] == '\n') {
      buffer[0] = '\0';
      break;
    }
    s.push_back(buffer[0]);
  }
  return s.length();
}

ssize_t Connection::connWrite(const char* buffer, size_t len) {
  ssize_t written = 0;
  while (written < len) {
    ssize_t n = write(fd, buffer + written, len - written);
    if (n == -1) {
      printf("write failed\n");
      return -1;
    }
    written += n;
  }
  return written;
}

ssize_t Connection::connSyncWrite(const char* buffer, size_t len,
                                  long timeout) {
  int r = el->aeWait(fd, ae::AeFlags::aeWritable, timeout);
  if (r < 0) {
    printf("conn sync write failed for connection fd %d\n", fd);
    return -1;
  } else if (r == 0) {
    printf("aeWait timeout\n");
    return 0;
  }
  printf("Conn start write sync %s\n", buffer);
  return connWrite(buffer, len);
}

ssize_t Connection::connWritev(
    const std::vector<std::pair<char*, size_t>>& mem_blocks) {
  iovec vec[mem_blocks.size()];
  ssize_t len = 0, written = 0;

  for (int i = 0; i < mem_blocks.size(); ++i) {
    vec[i].iov_base = mem_blocks[i].first;
    vec[i].iov_len = mem_blocks[i].second;
    len += mem_blocks[i].second;
  }

  return writev(fd, vec, mem_blocks.size());
}
}  // namespace connection
}  // namespace redis_simple
