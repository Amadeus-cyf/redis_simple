#include "connection.h"

#include <sys/uio.h>
#include <unistd.h>

#include <cstring>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "event_loop/ae_file_event.h"
#include "tcp/tcp.h"

namespace redis_simple::connection {
namespace {
int BindAndConnectTcp(const AddressInfo& remote,
                      const std::optional<AddressInfo>& local) {
  const tcp::TcpAddrInfo remote_addr(remote.ip, remote.port);
  const auto local_addr =
      local.has_value()
          ? std::make_optional<tcp::TcpAddrInfo>(local->ip, local->port)
          : std::nullopt;
  return tcp::TcpBindAndConnect(remote_addr, local_addr);
}
}  // namespace

Connection::Connection(const Context& ctx)
    : fd_(ctx.fd),
      el_(ctx.event_loop),
      state_(ConnectionState::kConnect),
      read_callback_(nullptr),
      write_callback_(nullptr) {}

ConnectionStatus Connection::BindAndConnect(
    const AddressInfo& remote, const std::optional<AddressInfo>& local) {
  if (auto* event_loop = el_) {
    int fd = BindAndConnectTcp(remote, local);
    if (fd < 0) {
      return ConnectionStatus::kError;
    }
    fd_ = fd;
    state_ = ConnectionState::kConnecting;
    auto event = ae::FileEvent::Create(nullptr, SocketEventCallback, this,
                                       ae::ToInt(ae::EventFlag::kWritable));
    if (event_loop->CreateFileEvent(fd_, std::move(event)) ==
        ae::EventLoopStatus::kError) {
      RS_LOG_DEBUG("adding connection socket event callback error");
      return ConnectionStatus::kError;
    }
  } else {
    RS_LOG_DEBUG("no event loop\n");
    return ConnectionStatus::kError;
  }
  return ConnectionStatus::kOk;
}

ConnectionStatus Connection::BindAndBlockingConnect(
    const AddressInfo& remote, const std::optional<AddressInfo>& local,
    long timeout) {
  int fd = BindAndConnectTcp(remote, local);
  if (fd < 0) {
    return ConnectionStatus::kError;
  }
  fd_ = fd;
  if (WaitWrite(timeout) <= 0) {
    RS_LOG_DEBUG("wait failed\n");
    fd_ = -1;
    return ConnectionStatus::kError;
  }
  state_ = ConnectionState::kConnected;
  return ConnectionStatus::kOk;
}

ConnectionStatus Connection::BindAndListen(const AddressInfo& addr_info) {
  int s = tcp::TcpCreateSocket(AF_INET, true);
  if (s < 0) {
    RS_LOG_DEBUG("create socket failed\n");
    return ConnectionStatus::kError;
  }
  const tcp::TcpAddrInfo tcp_addr(addr_info.ip, addr_info.port);
  if (tcp::TcpBind(s, tcp_addr) == tcp::TcpStatusCode::kError) {
    return ConnectionStatus::kError;
  }
  if (tcp::TcpListen(s) == tcp::TcpStatusCode::kError) {
    return ConnectionStatus::kError;
  }
  fd_ = s;
  return ConnectionStatus::kOk;
}

ConnectionStatus Connection::Accept(AddressInfo* const addr_info) {
  if (fd_ < 0 || state_ != ConnectionState::kAccepting) {
    return ConnectionStatus::kError;
  }
  tcp::TcpAddrInfo tcp_addr;
  int fd = tcp::TcpAccept(fd_, &tcp_addr);
  if (fd < 0) {
    return ConnectionStatus::kError;
  }
  addr_info->ip = tcp_addr.ip;
  addr_info->port = tcp_addr.port;
  RS_LOG_DEBUG("accept %s %d\n", addr_info->ip.c_str(), addr_info->port);
  state_ = ConnectionState::kConnected;
  fd_ = fd;
  RS_LOG_DEBUG("conn state accept %d\n", state_);
  return ConnectionStatus::kOk;
}

bool Connection::SetReadCallback(ConnectionCallback read_callback) {
  if (!read_callback) {
    return UnsetReadCallback();
  }
  if (auto* event_loop = el_) {
    auto event = ae::FileEvent::Create(SocketEventCallback, nullptr, this,
                                       ae::ToInt(ae::EventFlag::kReadable));
    if (event_loop->CreateFileEvent(fd_, std::move(event)) ==
        ae::EventLoopStatus::kError) {
      RS_LOG_DEBUG("failed to set read callback\n");
      return false;
    }
    read_callback_ = std::move(read_callback);
    flags_ |= ae::EventFlag::kReadable;
  } else {
    RS_LOG_DEBUG("no event loop\n");
    return false;
  }
  return true;
}

bool Connection::UnsetReadCallback() {
  if (auto* event_loop = el_) {
    if (event_loop->DeleteFileEvent(fd_, ae::EventFlag::kReadable) ==
        ae::EventLoopStatus::kError) {
      RS_LOG_DEBUG("failed to unset read callback\n");
      return false;
    }
    read_callback_ = nullptr;
    flags_ &= ~ae::EventFlag::kReadable;
  } else {
    RS_LOG_DEBUG("no event loop\n");
    return false;
  }
  return true;
}

bool Connection::SetWriteCallback(ConnectionCallback callback, bool barrier) {
  if (barrier) {
    flags_ |= kWriteBarrier;
  } else {
    flags_ &= ~kWriteBarrier;
  }
  if (!callback) {
    return UnsetWriteCallback();
  }
  if (auto* event_loop = el_) {
    auto event = ae::FileEvent::Create(nullptr, SocketEventCallback, this,
                                       ae::ToInt(ae::EventFlag::kWritable));
    if (event_loop->CreateFileEvent(fd_, std::move(event)) ==
        ae::EventLoopStatus::kError) {
      RS_LOG_DEBUG("failed to set write callback\n");
      return false;
    }
    write_callback_ = std::move(callback);
    flags_ |= ae::EventFlag::kWritable;
  } else {
    RS_LOG_DEBUG("no event loop\n");
    return false;
  }
  return true;
}

bool Connection::UnsetWriteCallback() {
  if (auto* event_loop = el_) {
    if (event_loop->DeleteFileEvent(fd_, ae::EventFlag::kWritable) ==
        ae::EventLoopStatus::kError) {
      RS_LOG_DEBUG("failed to unset write callback\n");
      return false;
    }
    write_callback_ = nullptr;
    flags_ &= ~ae::EventFlag::kWritable;
  } else {
    RS_LOG_DEBUG("no event loop\n");
    return false;
  }
  return true;
}

ssize_t Connection::Read(char* const buffer, size_t readlen) const {
  ssize_t r = read(fd_, buffer, readlen);
  if (r < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnectionState::kConnected) {
      state_ = ConnectionState::kError;
    }
    return -1;
  }
  if (r == 0) {
    state_ = ConnectionState::kClosed;
  }
  return r;
}

ssize_t Connection::BatchRead(std::string& s) const {
  char buffer[1024];
  ssize_t r = 0;
  while ((r = read(fd_, buffer, sizeof(buffer))) != EOF) {
    RS_LOG_DEBUG("read %zu\n", r);
    if (r == 0) {
      break;
    }
    s.append(buffer, r);
  }
  if (s.empty() && r < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnectionState::kConnected) {
      state_ = ConnectionState::kError;
    }
    return -1;
  }
  if (s.empty() && r == 0) {
    state_ = ConnectionState::kClosed;
  }
  if (s.size() > static_cast<size_t>(std::numeric_limits<ssize_t>::max())) {
    return -1;
  }
  return static_cast<ssize_t>(s.size());
}

ssize_t Connection::SyncRead(char* buffer, size_t readlen, long timeout) const {
  // Try the syscall first so already-ready sockets avoid an extra poll().
  ssize_t r = read(fd_, buffer, readlen);
  if (r > 0) {
    return r;
  }
  if (r == 0) {
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
    if (errno != EINTR && state_ == ConnectionState::kConnected) {
      state_ = ConnectionState::kError;
    }
    return -1;
  }
  if (s.empty() && r == 0) {
    state_ = ConnectionState::kClosed;
  }
  if (s.size() > static_cast<size_t>(std::numeric_limits<ssize_t>::max())) {
    return -1;
  }
  return static_cast<ssize_t>(s.size());
}

ssize_t Connection::Write(const char* buffer, size_t len) const {
  ssize_t n = write(fd_, buffer, len);
  if (n < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnectionState::kConnected) {
      state_ = ConnectionState::kError;
    }
    RS_LOG_DEBUG("write failed\n");
    return -1;
  }
  return n;
}

ssize_t Connection::SyncWrite(const char* buffer, size_t len,
                              long timeout) const {
  // Try the syscall first so already-ready sockets avoid an extra poll().
  ssize_t r = write(fd_, buffer, len);
  if (r > 0) {
    buffer += r;
    len -= r;
  } else if (r < 0 && errno != EAGAIN) {
    return -1;
  }
  if (len == 0) {
    return r;
  }
  if (WaitWrite(timeout) < 0) {
    RS_LOG_DEBUG("wait failed\n");
    return -1;
  }
  return Write(buffer, len);
}

ssize_t Connection::WriteVector(
    const std::vector<std::pair<char*, size_t>>& mem_blocks) const {
  if (mem_blocks.size() >
      static_cast<size_t>(std::numeric_limits<int>::max())) {
    return -1;
  }
  std::vector<iovec> vec(mem_blocks.size());
  for (size_t i = 0; i < mem_blocks.size(); ++i) {
    vec[i].iov_base = mem_blocks[i].first;
    vec[i].iov_len = mem_blocks[i].second;
  }
  ssize_t n = writev(fd_, vec.data(), static_cast<int>(vec.size()));
  if (n < 0 && errno != EAGAIN) {
    if (errno != EINTR && state_ == ConnectionState::kConnected) {
      state_ = ConnectionState::kError;
    }
    RS_LOG_DEBUG("write failed\n");
    return -1;
  }
  return n;
}

ae::EventCallbackStatus Connection::SocketEventCallback(ae::EventLoop* el,
                                                        int fd,
                                                        Connection* conn,
                                                        int mask) {
  RS_LOG_DEBUG(
      "event callback called with fd = %d, mask_read = %d, mask_write = %d\n",
      fd, mask & ae::EventFlag::kReadable, mask & ae::EventFlag::kWritable);
  if (conn == nullptr) {
    return ae::EventCallbackStatus::kError;
  }
  RS_LOG_DEBUG("state: %d\n", conn->State());
  if (conn->State() == ConnectionState::kConnecting &&
      ((mask & ae::EventFlag::kWritable) != 0)) {
    // A nonblocking connect completes through the writable event; SO_ERROR
    // tells whether it succeeded.
    if (tcp::IsSocketError(fd)) {
      RS_LOG_DEBUG("socket error\n");
      conn->SetState(ConnectionState::kError);
      return ae::EventCallbackStatus::kError;
    }
    RS_LOG_DEBUG("connection state set to connected\n");
    conn->SetState(ConnectionState::kConnected);
  }
  int invert = conn->flags_ & kWriteBarrier;
  // Normal order is read-before-write. A write barrier lets command replies
  // flush before another readable event queues more work on the same socket.
  if ((invert == 0) && ((mask & ae::EventFlag::kReadable) != 0) &&
      conn->read_callback_) {
    conn->read_callback_(conn);
  }
  if (((mask & ae::EventFlag::kWritable) != 0) && conn->write_callback_) {
    conn->write_callback_(conn);
  }
  if ((invert != 0) && ((mask & ae::EventFlag::kReadable) != 0) &&
      conn->read_callback_) {
    conn->read_callback_(conn);
  }
  return ae::EventCallbackStatus::kOk;
}

int Connection::Wait(ae::EventFlag flag, long timeout) const {
  int r = ae::WaitForEvent(fd_, flag, timeout);
  if (r < 0) {
    RS_LOG_DEBUG("conn sync %s failed for connection %d\n",
                 flag == ae::EventFlag::kReadable ? "read" : "write", fd_);
    return -1;
  }
  if (r == 0) {
    RS_LOG_DEBUG("aeWait timeout\n");
    return -1;
  }
  return r;
}
}  // namespace redis_simple::connection
