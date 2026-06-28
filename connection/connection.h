#pragma once

#include <unistd.h>

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "connection/connection_callback.h"
#include "event_loop/ae.h"

namespace redis_simple::connection {
enum class ConnectionStatus {
  kOk = 0,
  kError = -1,
};

enum class ConnectionState {
  kConnect = 1,
  kConnecting = 1 << 1,
  kAccepting = 1 << 2,
  kHandshake = 1 << 3,
  kConnected = 1 << 4,
  kError = 1 << 5,
  kClosed = 1 << 6,
};

struct Context {
  ae::EventLoop* event_loop{nullptr};
  int fd{-1};
};

struct AddressInfo {
  std::string ip;
  int port;
  AddressInfo() : ip(""), port(0) {}
  AddressInfo(const std::string& ip, int port) : ip(ip), port(port) {}
};

class Connection {
 public:
  explicit Connection(const Context& ctx);
  ConnectionStatus BindAndConnect(const AddressInfo& remote,
                                  const std::optional<AddressInfo>& local);
  ConnectionStatus BindAndBlockingConnect(
      const AddressInfo& remote, const std::optional<AddressInfo>& local,
      long timeout);
  ConnectionStatus BindAndListen(const AddressInfo& addr_info);
  ConnectionStatus Accept(AddressInfo* const addr_info);
  bool SetReadCallback(ConnectionCallback callback);
  bool UnsetReadCallback();
  bool HasReadCallback() const { return static_cast<bool>(read_callback_); }
  bool SetWriteCallback(ConnectionCallback callback, bool barrier = false);
  bool UnsetWriteCallback();
  bool HasWriteCallback() const { return static_cast<bool>(write_callback_); }
  int Descriptor() const { return fd_; }
  ConnectionState State() const { return state_; }
  void SetState(ConnectionState state) { state_ = state; }
  void SetPrivateData(std::any private_data) { private_data_ = private_data; }
  std::any PrivateData() const { return private_data_; }
  ssize_t Read(char* const buf, size_t readlen) const;
  ssize_t BatchRead(std::string& s) const;
  ssize_t SyncRead(char* buffer, size_t readlen, long timeout) const;
  ssize_t SyncBatchRead(std::string& s, long timeout) const;
  ssize_t SyncReadline(std::string& s, long timeout) const;
  ssize_t Write(const char* buffer, size_t len) const;
  ssize_t WriteVector(
      const std::vector<std::pair<char*, size_t>>& mem_blocks) const;
  ssize_t SyncWrite(const char* buffer, size_t len, long timeout) const;
  void Close() {
    if (fd_ >= 0) {
      close(fd_);
      fd_ = -1;
    }
  }
  ~Connection() { Close(); }

 private:
  // Give pending writes priority over reads for this connection.
  static constexpr int kWriteBarrier = 1;

  static ae::EventCallbackStatus SocketEventCallback(ae::EventLoop* el, int fd,
                                                     Connection* conn,
                                                     int mask);
  int WaitRead(long timeout) const {
    return Wait(ae::EventFlag::kReadable, timeout);
  }
  int WaitWrite(long timeout) const {
    return Wait(ae::EventFlag::kWritable, timeout);
  }
  int Wait(ae::EventFlag flag, long timeout) const;
  int fd_;
  int flags_{};
  mutable ConnectionState state_;
  std::any private_data_;
  ae::EventLoop* el_{nullptr};
  ConnectionCallback read_callback_;
  ConnectionCallback write_callback_;
};
}  // namespace redis_simple::connection
