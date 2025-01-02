#pragma once

#include <unistd.h>

#include <any>
#include <optional>
#include <string>

#include "connection/conn_handler.h"
#include "event_loop/ae.h"

namespace redis_simple {
namespace connection {
enum class StatusCode {
  connStatusOK = 0,
  connStatusErr = -1,
};

enum class ConnState {
  connStateConnect = 1,
  connStateConnecting = 1 << 1,
  connStateAccepting = 1 << 2,
  connStateHandshake = 1 << 3,
  connStateConnected = 1 << 4,
  connStateError = 1 << 5,
  connStateClosed = 1 << 6,
};

struct Context {
  std::weak_ptr<ae::AeEventLoop> event_loop;
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
  StatusCode BindAndConnect(const AddressInfo& remote,
                            const std::optional<const AddressInfo>& local);
  StatusCode BindAndBlockingConnect(
      const AddressInfo& remote, const std::optional<const AddressInfo>& local,
      long timeout);
  StatusCode BindAndListen(const AddressInfo& addrInfo);
  StatusCode Accept(AddressInfo* const addrInfo);
  bool SetReadHandler(std::unique_ptr<ConnHandler> handler);
  bool UnsetReadHandler();
  bool HasReadHandler() const { return read_handler_ != nullptr; }
  bool SetWriteHandler(std::unique_ptr<ConnHandler> handler,
                       bool barrier = false);
  bool UnsetWriteHandler();
  bool HasWriteHandler() const { return write_handler_ != nullptr; }
  int Fd() const { return fd_; }
  ConnState State() const { return state_; }
  void SetState(ConnState state) { state_ = state; }
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
  // If this flag is set, then write handler will be called before the read
  // handler.
  static constexpr int connFlagWriteBarrier = 1;

  static ae::AeEventStatus ConnSocketEventHandler(ae::AeEventLoop* el, int fd,
                                                  Connection* client_data,
                                                  int mask);
  int WaitRead(long timeout) const {
    return Wait(ae::AeFlags::aeReadable, timeout);
  }
  int WaitWrite(long timeout) const {
    return Wait(ae::AeFlags::aeWritable, timeout);
  }
  int Wait(ae::AeFlags flag, long timeout) const;
  int fd_;
  // Flags used to judge connFlagWriteBarrier is set
  int flags_;
  // Connection state
  mutable ConnState state_;
  // Data used by connetion handlers
  std::any private_data_;
  // Event loop
  std::weak_ptr<ae::AeEventLoop> el_;
  // Connection read handler
  std::unique_ptr<ConnHandler> read_handler_;
  // Connection write handler
  std::unique_ptr<ConnHandler> write_handler_;
  // Connection accepted handler
  std::unique_ptr<ConnHandler> accept_handler_;
};
}  // namespace connection
}  // namespace redis_simple
