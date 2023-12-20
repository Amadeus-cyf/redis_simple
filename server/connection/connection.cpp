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

StatusCode Connection::Connect(const std::string& remote_ip, int remote_port,
                               const std::string& local_ip, int local_port) {
  if (!el) {
    return StatusCode::c_err;
  }
  std::optional<std::string> opt_ip = std::make_optional<std::string>(local_ip);
  std::optional<int> opt_port = std::make_optional<int>(local_port);
  int s = tcp::TCP_Connect(remote_ip, remote_port, true, opt_ip, opt_port);
  if (s == -1) {
    return StatusCode::c_err;
  }
  fd = s;
  state = ConnState::connStateConnecting;
  ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::Create(
      nullptr, ConnSocketEventHandler, this, ae::AeFlags::aeWritable);
  if (el->AeCreateFileEvent(fd, e) < 0) {
    printf("adding connection socket event handler error");
    return StatusCode::c_err;
  }
  return StatusCode::c_ok;
}

StatusCode Connection::Listen(const std::string& ip, int port) {
  int s = tcp::TCP_CreateSocket(AF_INET, true);
  if (s < 0) {
    printf("create socket failed\n");
    return StatusCode::c_err;
  }
  if (tcp::TCP_BindAndListen(s, ip, port) == tcp::TCPStatusCode::tcpError) {
    return StatusCode::c_err;
  }
  fd = s;
  return StatusCode::c_ok;
}

StatusCode Connection::Accept(std::string* remote_ip, int* remote_port) {
  if (fd < 0) {
    printf("socket not created\n");
    return StatusCode::c_err;
  }
  int s = tcp::TCP_Accept(fd, remote_ip, remote_port);
  if (s < 0) {
    return StatusCode::c_err;
  }
  if (state != ConnState::connStateAccepting) {
    return StatusCode::c_err;
  }
  state = ConnState::connStateConnected;
  fd = s;
  if (accept_handler) {
    accept_handler->Handle(this);
  }
  printf("conn state accept %d\n", state);
  return StatusCode::c_ok;
}

void Connection::SetReadHandler(std::unique_ptr<ConnHandler> rHandler) {
  if (!rHandler) {
    UnsetReadHandler();
  } else {
    ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::Create(
        ConnSocketEventHandler, nullptr, this, ae::AeFlags::aeReadable);
    el->AeCreateFileEvent(fd, e);
    read_handler = std::move(rHandler);
    flags |= ae::AeFlags::aeReadable;
  }
}

void Connection::UnsetReadHandler() {
  read_handler = nullptr;
  el->AeDeleteFileEvent(fd, ae::AeFlags::aeReadable);
  flags &= ~ae::AeFlags::aeReadable;
}

void Connection::SetWriteHandler(std::unique_ptr<ConnHandler> handler,
                                 bool barrier) {
  if (barrier) {
    flags |= connFlagWriteBarrier;
  } else {
    flags &= ~connFlagWriteBarrier;
  }
  _setWriteHandler(std::move(handler));
}

void Connection::_setWriteHandler(std::unique_ptr<ConnHandler> handler) {
  if (!handler) {
    UnsetWriteHandler();
  } else {
    ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::Create(
        nullptr, ConnSocketEventHandler, this, ae::AeFlags::aeWritable);
    el->AeCreateFileEvent(fd, e);
    write_handler = std::move(handler);
    flags |= ae::AeFlags::aeWritable;
  }
}

void Connection::UnsetWriteHandler() {
  write_handler = nullptr;
  el->AeDeleteFileEvent(fd, ae::AeFlags::aeWritable);
  flags &= ~ae::AeFlags::aeWritable;
}

ae::AeEventStatus Connection::ConnSocketEventHandler(ae::AeEventLoop* el,
                                                     int fd, Connection* conn,
                                                     int mask) {
  printf("event handler called with fd = %d, mask_read = %d, mask_write = %d\n",
         fd, mask & ae::AeFlags::aeReadable, mask & ae::AeFlags::aeWritable);
  if (conn == nullptr) {
    return ae::AeEventStatus::aeEventErr;
  }

  printf("state: %d\n", conn->State());
  if (conn->State() == ConnState::connStateConnecting &&
      (mask & ae::AeFlags::aeWritable)) {
    if (tcp::IsSocketError(fd)) {
      printf("socker error\n");
      conn->SetState(ConnState::connStateError);
      return ae::AeEventStatus::aeEventErr;
    } else {
      printf("connection state set to connected\n");
      conn->SetState(ConnState::connStateConnected);
    }
  }

  int invert = conn->flags & connFlagWriteBarrier;
  if (!invert && (mask & ae::AeFlags::aeReadable) && conn->read_handler) {
    conn->read_handler->Handle(conn);
  }
  if ((mask & ae::AeFlags::aeWritable) && conn->write_handler) {
    conn->write_handler->Handle(conn);
  }
  if (invert && (mask & ae::AeFlags::aeReadable) && conn->read_handler) {
    conn->read_handler->Handle(conn);
  }
  return ae::AeEventStatus::aeEventOK;
}

ssize_t Connection::Read(const char* buffer, size_t readlen) {
  return std::as_const(*this).Read(buffer, readlen);
}

ssize_t Connection::Read(const char* buffer, size_t readlen) const {
  ssize_t nread = 0;
  int r = read(fd, (char*)buffer, readlen);
  if (r < 0 && errno != EAGAIN) {
    if (errno != EINTR && state == ConnState::connStateConnected) {
      state = ConnState::connStateError;
    }
  } else if (r == 0) {
    state = ConnState::connStateClosed;
  }
  return r;
}

ssize_t Connection::Read(std::string& s) {
  return std::as_const(*this).Read(s);
}

ssize_t Connection::Read(std::string& s) const {
  char buffer[1024];
  int r = 0, nread = 0;
  while ((r = read(fd, buffer + nread, 1024)) != EOF) {
    if (r == 0) {
      break;
    }
    s.append(buffer, r);
    nread += r;
  }
  if (s.empty() && r < 0 && errno != EAGAIN) {
    if (errno != EINTR && state == ConnState::connStateConnected) {
      state = ConnState::connStateError;
    }
    return -1;
  } else if (s.empty() && r == 0) {
    state = ConnState::connStateClosed;
  }
  return s.size();
}

ssize_t Connection::SyncRead(const char* buffer, size_t readlen, long timeout) {
  return std::as_const(*this).SyncRead(buffer, readlen, timeout);
}

ssize_t Connection::SyncRead(const char* buffer, size_t readlen,
                             long timeout) const {
  int r = el->AeWait(fd, ae::AeFlags::aeReadable, timeout);
  if (r < 0) {
    printf("conn sync read failed for connection %d\n", fd);
    return -1;
  } else if (r == 0) {
    printf("aeWait timeout\n");
    return 0;
  }
  return Read(buffer, readlen);
}

ssize_t Connection::SyncRead(std::string& s, long timeout) {
  return std::as_const(*this).SyncRead(s, timeout);
}

ssize_t Connection::SyncRead(std::string& s, long timeout) const {
  int r = el->AeWait(fd, ae::AeFlags::aeReadable, timeout);
  if (r < 0) {
    printf("conn sync read string failed for connection %d\n", fd);
    return -1;
  } else if (r == 0) {
    printf("aeWait timeout\n");
    return 0;
  }
  printf("Conn start read sync\n");
  return Read(s);
}

ssize_t Connection::SyncReadline(std::string& s, long timeout) {
  return std::as_const(*this).SyncReadline(s, timeout);
}

ssize_t Connection::SyncReadline(std::string& s, long timeout) const {
  int r = el->AeWait(fd, ae::AeFlags::aeReadable, timeout);
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
    if (errno != EINTR && state == ConnState::connStateConnected) {
      state = ConnState::connStateError;
    }
  } else if (s.empty() && r == 0) {
    state = ConnState::connStateClosed;
  }
  return r < 0 ? r : s.size();
}

ssize_t Connection::Write(const char* buffer, size_t len) {
  return std::as_const(*this).Write(buffer, len);
}

ssize_t Connection::Write(const char* buffer, size_t len) const {
  ssize_t n = write(fd, buffer, len);
  if (n < 0 && errno != EAGAIN) {
    if (errno != EINTR && state == ConnState::connStateConnected) {
      state = ConnState::connStateError;
    }
    printf("write failed\n");
  }
  return n;
}

ssize_t Connection::SyncWrite(const char* buffer, size_t len, long timeout) {
  return std::as_const(*this).SyncWrite(buffer, len, timeout);
}

ssize_t Connection::SyncWrite(const char* buffer, size_t len,
                              long timeout) const {
  int r = el->AeWait(fd, ae::AeFlags::aeWritable, timeout);
  if (r < 0) {
    printf("conn sync write failed for connection fd %d\n", fd);
    return -1;
  } else if (r == 0) {
    printf("aeWait timeout\n");
    return 0;
  }
  printf("Conn start write sync %s\n", buffer);
  return Write(buffer, len);
}

ssize_t Connection::Writev(
    const std::vector<std::pair<char*, size_t>>& mem_blocks) {
  return std::as_const(*this).Writev(mem_blocks);
}

ssize_t Connection::Writev(
    const std::vector<std::pair<char*, size_t>>& mem_blocks) const {
  iovec vec[mem_blocks.size()];
  ssize_t len = 0, written = 0;
  for (int i = 0; i < mem_blocks.size(); ++i) {
    vec[i].iov_base = mem_blocks[i].first;
    vec[i].iov_len = mem_blocks[i].second;
    len += mem_blocks[i].second;
  }
  ssize_t n = writev(fd, vec, mem_blocks.size());
  if (n < 0 && errno != EAGAIN) {
    if (errno != EINTR && state == ConnState::connStateConnected) {
      state = ConnState::connStateError;
    }
    printf("write failed\n");
  }
  return n;
}
}  // namespace connection
}  // namespace redis_simple
