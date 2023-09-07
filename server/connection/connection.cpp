#include "connection.h"

#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include "event_loop/ae_file_event_impl.h"
#include "tcp/tcp.h"

namespace redis_simple {
namespace connection {
Connection::Connection(const Context& ctx)
    : fd(ctx.fd),
      el(ctx.loop),
      state(ConnState::connStateConnect),
      read_handler(nullptr),
      write_handler(nullptr),
      accept_handler(nullptr) {}

StatusCode Connection::connect(const std::string& remote_ip, int remote_port,
                               const std::string& local_ip, int local_port) {
  if (!el) {
    return StatusCode::c_err;
  }
  std::optional<std::string> opt_ip = std::make_optional<std::string>(local_ip);
  std::optional<int> opt_port = std::make_optional<int>(local_port);
  int s = tcp::tcpConnect(remote_ip, remote_port, true, opt_ip, opt_port);
  if (s == -1) {
    return StatusCode::c_err;
  }
  fd = s;
  state = ConnState::connStateConnecting;
  ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::create(
      nullptr, connSocketEventHandler, this, ae::AeFlags::aeWritable);
  if (el->aeCreateFileEvent(fd, e) < 0) {
    printf("adding connection socket event handler error");
    return StatusCode::c_err;
  }
  return StatusCode::c_ok;
}

StatusCode Connection::listen(const std::string& ip, int port) {
  int s = tcp::tcpCreateSocket(AF_INET, true);
  if (s < 0) {
    printf("create socket failed\n");
    return StatusCode::c_err;
  }
  if (tcp::tcpBindAndListen(s, ip, port) == tcp::TCPStatusCode::tcpError) {
    return StatusCode::c_err;
  }
  fd = s;
  return StatusCode::c_ok;
}

StatusCode Connection::accept(std::string* remote_ip, int* remote_port) {
  if (fd < 0) {
    printf("socket not created\n");
    return StatusCode::c_err;
  }
  int s = tcp::tcpAccept(fd, remote_ip, remote_port);
  if (s < 0) {
    return StatusCode::c_err;
  }
  if (state != ConnState::connStateAccepting) {
    return StatusCode::c_err;
  }
  state = ConnState::connStateConnected;
  fd = s;
  if (accept_handler) {
    accept_handler->handle(this);
  }
  printf("conn state accept %d\n", state);
  return StatusCode::c_ok;
}

void Connection::setReadHandler(std::unique_ptr<ConnHandler> rHandler) {
  if (!rHandler) {
    unsetReadHandler();
  } else {
    ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::create(
        connSocketEventHandler, nullptr, this, ae::AeFlags::aeReadable);
    el->aeCreateFileEvent(fd, e);
    read_handler = std::move(rHandler);
    flags |= ae::AeFlags::aeReadable;
  }
}

void Connection::unsetReadHandler() {
  read_handler = nullptr;
  el->aeDeleteFileEvent(fd, ae::AeFlags::aeReadable);
  flags &= ~ae::AeFlags::aeReadable;
}

void Connection::setWriteHandler(std::unique_ptr<ConnHandler> wHandler) {
  if (!wHandler) {
    unsetWriteHandler();
  } else {
    ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::create(
        nullptr, connSocketEventHandler, this, ae::AeFlags::aeWritable);
    el->aeCreateFileEvent(fd, e);
    write_handler = std::move(wHandler);
    flags |= ae::AeFlags::aeWritable;
  }
}

void Connection::unsetWriteHandler() {
  write_handler = nullptr;
  el->aeDeleteFileEvent(fd, ae::AeFlags::aeWritable);
  flags &= ~ae::AeFlags::aeWritable;
}

ae::AeEventStatus Connection::connSocketEventHandler(ae::AeEventLoop* el,
                                                     int fd, Connection* conn,
                                                     int mask) {
  printf("event handler called with fd = %d, mask_read = %d, mask_write = %d\n",
         fd, mask & ae::AeFlags::aeReadable, mask & ae::AeFlags::aeWritable);
  if (conn == nullptr) {
    return ae::AeEventStatus::aeEventErr;
  }

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
  return std::as_const(*this).connRead(buffer, readlen);
}

ssize_t Connection::connRead(const char* buffer, size_t readlen) const {
  ssize_t nread = 0;
  int r = 0;
  while (nread < readlen &&
         (r = read(fd, (char*)buffer + nread, 1024)) != EOF) {
    if (!r) {
      break;
    }
    nread += r;
  }
  if (r < 0 && errno != EAGAIN) {
    state = ConnState::connStateError;
    return -1;
  }
  if (nread == 0) {
    state = ConnState::connStateClosed;
  }
  return nread;
}

ssize_t Connection::connRead(std::string& s) {
  return std::as_const(*this).connRead(s);
}

ssize_t Connection::connRead(std::string& s) const {
  char buffer[1024];
  int r = 0;
  int nread = 0;
  while ((r = read(fd, buffer + nread, 1024)) != EOF) {
    if (r == 0) {
      break;
    }
    s.append(buffer, r);
  }
  if (r < 0 && errno != EAGAIN) {
    state = ConnState::connStateError;
    return -1;
  }
  if (s.size() == 0) {
    state = ConnState::connStateClosed;
  }
  return s.length();
}

ssize_t Connection::connSyncRead(const char* buffer, size_t readlen,
                                 long timeout) {
  return std::as_const(*this).connSyncRead(buffer, readlen, timeout);
}

ssize_t Connection::connSyncRead(const char* buffer, size_t readlen,
                                 long timeout) const {
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
  return std::as_const(*this).connSyncRead(s, timeout);
}

ssize_t Connection::connSyncRead(std::string& s, long timeout) const {
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
  return std::as_const(*this).connSyncReadline(s, timeout);
}

ssize_t Connection::connSyncReadline(std::string& s, long timeout) const {
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
    if (r == 0 || buffer[0] == '\n') {
      break;
    }
    s.push_back(buffer[0]);
  }
  if (r < 0 && errno != EAGAIN) {
    state = ConnState::connStateError;
    return -1;
  }
  if (s.size() == 0) {
    state = ConnState::connStateClosed;
  }
  return s.size();
}

ssize_t Connection::connWrite(const char* buffer, size_t len) {
  return std::as_const(*this).connWrite(buffer, len);
}

ssize_t Connection::connWrite(const char* buffer, size_t len) const {
  ssize_t written = 0;
  while (written < len) {
    ssize_t n = write(fd, buffer + written, len - written);
    if (n < 0) {
      if (errno != EINTR && state == ConnState::connStateConnected) {
        state = ConnState::connStateError;
      }
      printf("write failed\n");
      return n;
    }
    written += n;
  }
  return written;
}

ssize_t Connection::connSyncWrite(const char* buffer, size_t len,
                                  long timeout) {
  return std::as_const(*this).connSyncWrite(buffer, len, timeout);
}

ssize_t Connection::connSyncWrite(const char* buffer, size_t len,
                                  long timeout) const {
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
  return std::as_const(*this).connWritev(mem_blocks);
}

ssize_t Connection::connWritev(
    const std::vector<std::pair<char*, size_t>>& mem_blocks) const {
  iovec vec[mem_blocks.size()];
  ssize_t len = 0, written = 0;
  for (int i = 0; i < mem_blocks.size(); ++i) {
    vec[i].iov_base = mem_blocks[i].first;
    vec[i].iov_len = mem_blocks[i].second;
    len += mem_blocks[i].second;
  }
  ssize_t n = writev(fd, vec, mem_blocks.size());
  if (n < 0) {
    if (errno != EINTR && state == ConnState::connStateConnected) {
      state = ConnState::connStateError;
    }
    printf("write failed\n");
  }
  return n;
}
}  // namespace connection
}  // namespace redis_simple
