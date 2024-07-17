#include "connection.h"

#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <optional>

#include "event_loop/ae_file_event_impl.h"
#include "tcp/tcp.h"

namespace redis_simple {
namespace connection {
namespace {
int TCPBindAndConnect(const AddressInfo& remote,
                      const std::optional<const AddressInfo>& local) {
  const tcp::TCPAddrInfo remote_addr(remote.ip, remote.port);
  const std::optional<const tcp::TCPAddrInfo>& local_addr =
      local.has_value() ? std::make_optional<tcp::TCPAddrInfo>(
                              local.value().ip, local.value().port)
                        : std::nullopt;
  return tcp::TCP_BindAndConnect(remote_addr, local_addr);
}
}  // namespace

Connection::Connection(const Context& ctx)
    : fd_(ctx.fd),
      el_(ctx.event_loop),
      state_(ConnState::connStateConnect),
      read_handler_(nullptr),
      write_handler_(nullptr),
      accept_handler_(nullptr) {}

StatusCode Connection::BindAndConnect(
    const AddressInfo& remote, const std::optional<const AddressInfo>& local) {
  if (std::shared_ptr<ae::AeEventLoop> eventLoop = el_.lock()) {
    if (!eventLoop) {
      return StatusCode::connStatusErr;
    }
    int fd = TCPBindAndConnect(remote, local);
    if (fd < 0) {
      return StatusCode::connStatusErr;
    }
    fd_ = fd;
    state_ = ConnState::connStateConnecting;
    ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::Create(
        nullptr, ConnSocketEventHandler, this, ae::AeFlags::aeWritable);
    if (eventLoop->AeCreateFileEvent(fd_, e) < 0) {
      printf("adding connection socket event handler error");
      return StatusCode::connStatusErr;
    }
  } else {
    printf("event loop expired\n");
    return StatusCode::connStatusErr;
  }
  return StatusCode::connStatusOK;
}

StatusCode Connection::BindAndBlockingConnect(
    const AddressInfo& remote, const std::optional<const AddressInfo>& local,
    long timeout) {
  int fd = TCPBindAndConnect(remote, local);
  if (fd < 0) {
    return StatusCode::connStatusErr;
  }
  fd_ = fd;
  if (WaitWrite(timeout) <= 0) {
    printf("wait failed\n");
    fd_ = -1;
    return StatusCode::connStatusErr;
  }
  state_ = ConnState::connStateConnected;
  return StatusCode::connStatusOK;
}

StatusCode Connection::BindAndListen(const AddressInfo& addrInfo) {
  int s = tcp::TCP_CreateSocket(AF_INET, true);
  if (s < 0) {
    printf("create socket failed\n");
    return StatusCode::connStatusErr;
  }
  const tcp::TCPAddrInfo tcp_addr(addrInfo.ip, addrInfo.port);
  if (tcp::TCP_Bind(s, tcp_addr) == tcp::TCPStatusCode::tcpError) {
    return StatusCode::connStatusErr;
  }
  if (tcp::TCP_Listen(s) == tcp::TCPStatusCode::tcpError) {
    return StatusCode::connStatusErr;
  }
  fd_ = s;
  return StatusCode::connStatusOK;
}

StatusCode Connection::Accept(AddressInfo* const addrInfo) {
  if (fd_ < 0 || state_ != ConnState::connStateAccepting) {
    return StatusCode::connStatusErr;
  }
  tcp::TCPAddrInfo tcp_addr;
  int fd = tcp::TCP_Accept(fd_, &tcp_addr);
  if (fd < 0) {
    return StatusCode::connStatusErr;
  }
  addrInfo->ip = tcp_addr.ip;
  addrInfo->port = tcp_addr.port;
  printf("accept %s %d\n", addrInfo->ip.c_str(), addrInfo->port);
  state_ = ConnState::connStateConnected;
  fd_ = fd;
  if (accept_handler_) {
    accept_handler_->Handle(this);
  }
  printf("conn state accept %d\n", state_);
  return StatusCode::connStatusOK;
}

/*
 * Set connection read handler if the given handler is non-null. Otherwise,
 * uninstall the current read handler. Return true if succeeded.
 */
bool Connection::SetReadHandler(std::unique_ptr<ConnHandler> rHandler) {
  if (!rHandler) {
    return UnsetReadHandler();
  }
  if (std::shared_ptr<ae::AeEventLoop> eventLoop = el_.lock()) {
    if (!eventLoop) {
      printf("no event loop\n");
      return false;
    }
    ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::Create(
        ConnSocketEventHandler, nullptr, this, ae::AeFlags::aeReadable);
    if (eventLoop->AeCreateFileEvent(fd_, e) == ae::AeStatus::aeErr) {
      printf("failed to set read handler\n");
      return false;
    }
    read_handler_ = std::move(rHandler);
    flags_ |= ae::AeFlags::aeReadable;
  } else {
    printf("event loop expired\n");
    return false;
  }
  return true;
}

/*
 * Unset connection read handler. Return true if succeeded.
 */
bool Connection::UnsetReadHandler() {
  if (std::shared_ptr<ae::AeEventLoop> eventLoop = el_.lock()) {
    if (!eventLoop) {
      printf("no event loop\n");
      return false;
    }
    if (eventLoop->AeDeleteFileEvent(fd_, ae::AeFlags::aeReadable) ==
        ae::AeStatus::aeErr) {
      printf("failed to unset read handler\n");
      return false;
    }
    read_handler_ = nullptr;
    flags_ &= ~ae::AeFlags::aeReadable;
  } else {
    printf("event loop expired\n");
    return false;
  }
  return true;
}

/*
 * Set connection write handler if the given handler is non-null. Otherwise,
 * uninstall the current write handler. Return true if succeeded.
 */
bool Connection::SetWriteHandler(std::unique_ptr<ConnHandler> handler,
                                 bool barrier) {
  if (barrier) {
    flags_ |= connFlagWriteBarrier;
  } else {
    flags_ &= ~connFlagWriteBarrier;
  }
  if (!handler) {
    return UnsetWriteHandler();
  }
  if (std::shared_ptr<ae::AeEventLoop> eventLoop = el_.lock()) {
    if (!eventLoop) {
      printf("no event loop\n");
      return false;
    }
    ae::AeFileEvent* e = ae::AeFileEventImpl<Connection>::Create(
        nullptr, ConnSocketEventHandler, this, ae::AeFlags::aeWritable);
    if (eventLoop->AeCreateFileEvent(fd_, e) == ae::AeStatus::aeErr) {
      printf("failed to set write handler\n");
      return false;
    }
    write_handler_ = std::move(handler);
    flags_ |= ae::AeFlags::aeWritable;
  } else {
    printf("event loop expired\n");
    return false;
  }
  return true;
}

/*
 * Unset connection write handler. Return true if succeeded.
 */
bool Connection::UnsetWriteHandler() {
  if (std::shared_ptr<ae::AeEventLoop> eventLoop = el_.lock()) {
    if (!eventLoop) {
      printf("no event loop\n");
      return false;
    }
    if (eventLoop->AeDeleteFileEvent(fd_, ae::AeFlags::aeWritable) ==
        ae::AeStatus::aeErr) {
      printf("failed to unset write handler\n");
      return false;
    }
    write_handler_ = nullptr;
    flags_ &= ~ae::AeFlags::aeWritable;
  } else {
    printf("event loop expired\n");
    return false;
  }
  return true;
}

ssize_t Connection::Read(char* const buffer, size_t readlen) const {
  ssize_t r = read(fd_, buffer, readlen);
  if (r < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnState::connStateConnected) {
      state_ = ConnState::connStateError;
    }
    return -1;
  } else if (r == 0) {
    state_ = ConnState::connStateClosed;
  }
  return r;
}

ssize_t Connection::BatchRead(std::string& s) const {
  char buffer[1024];
  ssize_t r = 0, nread = 0;
  while ((r = read(fd_, buffer + nread, 1024)) != EOF) {
    printf("read %zu\n", r);
    if (r == 0) {
      break;
    }
    s.append(buffer, r);
    nread += r;
  }
  if (s.empty() && r < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnState::connStateConnected) {
      state_ = ConnState::connStateError;
    }
    return -1;
  } else if (s.empty() && r == 0) {
    state_ = ConnState::connStateClosed;
  }
  return s.size();
}

ssize_t Connection::SyncRead(char* buffer, size_t readlen, long timeout) const {
  /* optimistically tried to read first */
  ssize_t r = read(fd_, buffer, readlen);
  if (r > 0) {
    return r;
  } else if (r == 0) {
    return 0;
  } else if (r < 0 && errno != EAGAIN) {
    return -1;
  }
  if (WaitRead(timeout) < 0) {
    return -1;
  }
  return Read(buffer, readlen);
}

ssize_t Connection::SyncBatchRead(std::string& s, long timeout) const {
  if (WaitRead(timeout) < 0) {
    return -1;
  }
  return BatchRead(s);
}

ssize_t Connection::SyncReadline(std::string& s, long timeout) const {
  if (WaitRead(timeout) < 0) {
    return -1;
  }
  ssize_t r = 0;
  char buffer[1];
  while ((r = read(fd_, buffer, 1)) != EOF) {
    if (r == 0 || buffer[0] == '\n') {
      break;
    }
    s.push_back(buffer[0]);
  }
  if (r < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnState::connStateConnected) {
      state_ = ConnState::connStateError;
    }
    return -1;
  } else if (s.empty() && r == 0) {
    state_ = ConnState::connStateClosed;
  }
  return s.size();
}

ssize_t Connection::Write(const char* buffer, size_t len) const {
  ssize_t n = write(fd_, buffer, len);
  if (n < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnState::connStateConnected) {
      state_ = ConnState::connStateError;
    }
    printf("write failed\n");
    return -1;
  }
  return n;
}

ssize_t Connection::SyncWrite(const char* buffer, size_t len,
                              long timeout) const {
  /* optimistically tried to write first */
  ssize_t r = write(fd_, buffer, len);
  if (r > 0) {
    buffer += r;
    len -= r;
  } else if (r < 0 && errno != EAGAIN) {
    return -1;
  }
  /* Return if write completes */
  if (len == 0) {
    return r;
  }
  if (WaitWrite(timeout) < 0) {
    printf("wait failed\n");
    return -1;
  }
  return Write(buffer, len);
}

ssize_t Connection::Writev(
    const std::vector<std::pair<char*, size_t>>& mem_blocks) const {
  iovec vec[mem_blocks.size()];
  ssize_t len = 0, written = 0;
  for (size_t i = 0; i < mem_blocks.size(); ++i) {
    vec[i].iov_base = mem_blocks[i].first;
    vec[i].iov_len = mem_blocks[i].second;
    len += mem_blocks[i].second;
  }
  ssize_t n = writev(fd_, vec, mem_blocks.size());
  if (n < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnState::connStateConnected) {
      state_ = ConnState::connStateError;
    }
    printf("write failed\n");
    return -1;
  }
  return n;
}

ae::AeEventStatus Connection::ConnSocketEventHandler(ae::AeEventLoop* el,
                                                     int fd, Connection* conn,
                                                     int mask) {
  printf("event handler called with fd = %d, mask_read = %d, mask_write = %d\n",
         fd, mask & ae::AeFlags::aeReadable, mask & ae::AeFlags::aeWritable);
  if (conn == nullptr) {
    return ae::AeEventStatus::aeEventErr;
  }
  /* update connection state to connected if the current state is connecting and
   * the socket is available for read */
  printf("state: %d\n", conn->State());
  if (conn->State() == ConnState::connStateConnecting &&
      (mask & ae::AeFlags::aeWritable)) {
    if (tcp::IsSocketError(fd)) {
      printf("socket error\n");
      conn->SetState(ConnState::connStateError);
      return ae::AeEventStatus::aeEventErr;
    } else {
      printf("connection state set to connected\n");
      conn->SetState(ConnState::connStateConnected);
    }
  }
  /* call read/write handlers */
  int invert = conn->flags_ & connFlagWriteBarrier;
  if (!invert && (mask & ae::AeFlags::aeReadable) && conn->read_handler_) {
    conn->read_handler_->Handle(conn);
  }
  if ((mask & ae::AeFlags::aeWritable) && conn->write_handler_) {
    conn->write_handler_->Handle(conn);
  }
  if (invert && (mask & ae::AeFlags::aeReadable) && conn->read_handler_) {
    conn->read_handler_->Handle(conn);
  }
  return ae::AeEventStatus::aeEventOK;
}

int Connection::Wait(ae::AeFlags flag, long timeout) const {
  int r = ae::AeWait(fd_, flag, timeout);
  if (r < 0) {
    printf("conn sync %s failed for connection %d\n",
           flag == ae::AeFlags::aeReadable ? "read" : "write", fd_);
    return -1;
  } else if (r == 0) {
    printf("aeWait timeout\n");
    return -1;
  }
  return r;
}
}  // namespace connection
}  // namespace redis_simple
