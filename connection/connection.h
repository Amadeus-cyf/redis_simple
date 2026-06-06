#pragma once

#include <unistd.h>

#include <any>
#include <optional>
#include <string>
#include <vector>

#include "connection/conn_handler.h"
#include "event_loop/ae.h"

namespace redis_simple {
namespace connection {
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
  std::weak_ptr<ae::EventLoop> event_loop;
  int fd;
};

struct AddressInfo {
  std::string ip;
  int port;
  AddressInfo() : ip(""), port(0){};
  AddressInfo(const std::string& ip, int port) : ip(ip), port(port){};
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
  bool SetReadHandler(std::unique_ptr<ConnHandler> handler);
  bool UnsetReadHandler();
  bool HasReadHandler() const { return read_handler_ != nullptr; }
  bool SetWriteHandler(std::unique_ptr<ConnHandler> handler,
                       bool barrier = false);
  bool UnsetWriteHandler();
  bool HasWriteHandler() const { return write_handler_ != nullptr; }
  int Fd() const { return fd_; }
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
  ssize_t Writev(const std::vector<std::pair<char*, size_t>>& mem_blocks) const;
  ssize_t SyncWrite(const char* buffer, size_t len, long timeout) const;
  void Close() const { close(fd_); }
  ~Connection() { Close(); }

 private:
  // Give pending writes priority over reads for this connection.
  static constexpr int kWriteBarrier = 1;

  static ae::EventHandlerStatus ConnSocketEventHandler(ae::EventLoop* el,
                                                       int fd,
                                                       Connection* client_data,
                                                       int mask);
  int WaitRead(long timeout) const {
    return Wait(ae::EventFlag::kReadable, timeout);
  }
  int WaitWrite(long timeout) const {
    return Wait(ae::EventFlag::kWritable, timeout);
  }
  int Wait(ae::EventFlag flag, long timeout) const;
  int fd_;
  // Flags used to judge kWriteBarrier is set
  int flags_;
  // Connection state
  mutable ConnectionState state_;
  // Data used by connection handlers
  std::any private_data_;
  // Event loop
  std::weak_ptr<ae::EventLoop> el_;
  // Connection read handler
  std::unique_ptr<ConnHandler> read_handler_;
  // Connection write handler
  std::unique_ptr<ConnHandler> write_handler_;
  // Connection accepted handler
  std::unique_ptr<ConnHandler> accept_handler_;
};
}  // namespace connection
}  // namespace redis_simple
