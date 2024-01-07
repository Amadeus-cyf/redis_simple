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
  StatusCode BindAndListen(const AddressInfo& addrInfo);
  StatusCode Accept(AddressInfo& addrInfo);
  bool SetReadHandler(std::unique_ptr<ConnHandler> handler);
  bool UnsetReadHandler();
  bool HasReadHandler() { return read_handler_ != nullptr; }
  bool HasReadHandler() const { return read_handler_ != nullptr; }
  bool SetWriteHandler(std::unique_ptr<ConnHandler> handler,
                       bool barrier = false);
  bool UnsetWriteHandler();
  bool HasWriteHandler() { return write_handler_ != nullptr; }
  bool HasWriteHandler() const { return write_handler_ != nullptr; }
  int Fd() { return fd_; }
  ConnState State() { return state_; }
  ConnState State() const { return state_; }
  void SetState(ConnState state) { state_ = state; }
  void SetPrivateData(std::any private_data) { private_data_ = private_data; }
  std::any PrivateData() { return private_data_; }
  std::any PrivateData() const { return private_data_; }
  ssize_t Read(char* const buffer, size_t readlen) {
    return std::as_const(*this).Read(buffer, readlen);
  }
  ssize_t Read(char* const buf, size_t readlen) const;
  ssize_t BatchRead(std::string& s) {
    return std::as_const(*this).BatchRead(s);
  }
  ssize_t BatchRead(std::string& s) const;
  ssize_t SyncRead(char* const buffer, size_t readlen, long timeout) {
    return std::as_const(*this).SyncRead(buffer, readlen, timeout);
  }
  ssize_t SyncRead(char* const buffer, size_t readlen, long timeout) const;
  ssize_t SyncBatchRead(std::string& s, long timeout) {
    return std::as_const(*this).SyncBatchRead(s, timeout);
  }
  ssize_t SyncBatchRead(std::string& s, long timeout) const;
  ssize_t SyncReadline(std::string& s, long timeout) {
    return std::as_const(*this).SyncReadline(s, timeout);
  }
  ssize_t SyncReadline(std::string& s, long timeout) const;
  ssize_t Write(const char* buffer, size_t len) {
    return std::as_const(*this).Write(buffer, len);
  }
  ssize_t Write(const char* buffer, size_t len) const;
  ssize_t Writev(const std::vector<std::pair<char*, size_t>>& mem_blocks) {
    return std::as_const(*this).Writev(mem_blocks);
  }
  ssize_t Writev(const std::vector<std::pair<char*, size_t>>& mem_blocks) const;
  ssize_t SyncWrite(const char* buffer, size_t len, long timeout) {
    return std::as_const(*this).SyncWrite(buffer, len, timeout);
  }
  ssize_t SyncWrite(const char* buffer, size_t len, long timeout) const;
  void Close() { close(fd_); }
  void Close() const { close(fd_); }
  ~Connection() { Close(); }

 private:
  /* if this flag is set, then write handler will be called before the read
   * handler */
  static constexpr int connFlagWriteBarrier = 1;

  static ae::AeEventStatus ConnSocketEventHandler(ae::AeEventLoop* el, int fd,
                                                  Connection* client_data,
                                                  int mask);
  ssize_t WaitRead(long timeout) const {
    return Wait(ae::AeFlags::aeReadable, timeout);
  }
  ssize_t WaitWrite(long timeout) const {
    return Wait(ae::AeFlags::aeWritable, timeout);
  }
  ssize_t Wait(ae::AeFlags flag, long timeout) const;
  int fd_;
  /* flags used to judge connFlagWriteBarrier is set */
  int flags_;
  /* connection state */
  mutable ConnState state_;
  /* data used by connetion handlers */
  std::any private_data_;
  /* event loop */
  std::weak_ptr<ae::AeEventLoop> el_;
  /* connection read handler */
  std::unique_ptr<ConnHandler> read_handler_;
  /* connection write handler */
  std::unique_ptr<ConnHandler> write_handler_;
  /* connection accepted handler */
  std::unique_ptr<ConnHandler> accept_handler_;
};
}  // namespace connection
}  // namespace redis_simple
