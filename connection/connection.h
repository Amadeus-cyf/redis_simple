#pragma once

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <any>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "connection/connection_callback.h"
#include "event_loop/loop.h"

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
  event_loop::Loop* loop{nullptr};
  int fd{-1};
};

struct AddressInfo {
  std::string ip;
  int port{};
  AddressInfo() = default;
  AddressInfo(std::string_view ip, int port) : ip(ip), port(port) {}
};

class Connection {
 public:
  explicit Connection(const Context& ctx);
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  Connection(Connection&&) = delete;
  Connection& operator=(Connection&&) = delete;
  ConnectionStatus BindAndConnect(const AddressInfo& remote,
                                  const std::optional<AddressInfo>& local);
  ConnectionStatus BindAndBlockingConnect(
      const AddressInfo& remote, const std::optional<AddressInfo>& local,
      int64_t timeout_ms);
  ConnectionStatus BindAndListen(const AddressInfo& addr_info);
  ConnectionStatus Accept(int listener_fd, AddressInfo* addr_info);
  bool SetReadCallback(ConnectionCallback callback);
  bool UnsetReadCallback();
  bool HasReadCallback() const { return static_cast<bool>(read_callback_); }
  bool SetWriteCallback(ConnectionCallback callback, bool barrier = false);
  bool UnsetWriteCallback();
  bool HasWriteCallback() const { return static_cast<bool>(write_callback_); }
  int Descriptor() const { return fd_; }
  ConnectionState State() const { return state_; }
  void SetState(ConnectionState state) { state_ = state; }
  void SetPrivateData(std::any private_data) {
    private_data_ = std::move(private_data);
  }
  const std::any& PrivateData() const { return private_data_; }
  ssize_t Read(char* const buf, size_t readlen) const;
  ssize_t BatchRead(std::string& s) const;
  ssize_t SyncRead(char* buffer, size_t readlen, int64_t timeout_ms) const;
  ssize_t SyncBatchRead(std::string& s, int64_t timeout_ms) const;
  ssize_t SyncReadline(std::string& s, int64_t timeout_ms) const;
  ssize_t Write(const char* buffer, size_t len) const;
  ssize_t WriteVector(const std::vector<iovec>& blocks) const;
  ssize_t SyncWrite(const char* buffer, size_t len, int64_t timeout_ms) const;
  void Close();
  ~Connection();

 private:
  static event_loop::CallbackStatus ReadableEventCallback(
      event_loop::Loop* loop, int fd, Connection* conn, int mask);
  static event_loop::CallbackStatus WritableEventCallback(
      event_loop::Loop* loop, int fd, Connection* conn, int mask);
  int WaitRead(int64_t timeout_ms) const {
    return Wait(event_loop::EventFlag::kReadable, timeout_ms);
  }
  int WaitWrite(int64_t timeout_ms) const {
    return Wait(event_loop::EventFlag::kWritable, timeout_ms);
  }
  int Wait(event_loop::EventFlag flag, int64_t timeout_ms) const;
  int fd_;
  int flags_{};
  mutable ConnectionState state_;
  std::any private_data_;
  event_loop::Loop* loop_{nullptr};
  ConnectionCallback read_callback_;
  ConnectionCallback write_callback_;
};
}  // namespace redis_simple::connection
