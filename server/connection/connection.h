#pragma once

#include <unistd.h>

#include <any>
#include <string>

#include "event_loop/ae.h"
#include "server/conn_handler/conn_handler.h"

namespace redis_simple {
namespace connection {
enum class StatusCode {
  c_ok = 0,
  c_err = -1,
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
  std::weak_ptr<ae::AeEventLoop> loop;
  int fd;
};

class Connection {
 public:
  explicit Connection(const Context& ctx);
  StatusCode Connect(const std::string& remote_ip, int remote_port,
                     const std::string& local_ip, int local_port);
  StatusCode Listen(const std::string& ip, int port);
  StatusCode Accept(std::string* remote_ip, int* remote_port);
  void SetReadHandler(std::unique_ptr<ConnHandler> handler);
  void UnsetReadHandler();
  bool HasReadHandler() { return read_handler_ != nullptr; }
  bool HasReadHandler() const { return read_handler_ != nullptr; }
  void SetWriteHandler(std::unique_ptr<ConnHandler> handler,
                       bool barrier = false);
  void UnsetWriteHandler();
  bool HasWriteHandler() { return write_handler_ != nullptr; }
  bool HasWriteHandler() const { return write_handler_ != nullptr; }
  int Fd() { return fd_; }
  ConnState State() { return state_; }
  ConnState State() const { return state_; }
  void SetState(ConnState state) { state_ = state; }
  void SetPrivateData(std::any private_data) { private_data_ = private_data; }
  std::any PrivateData() { return private_data_; }
  std::any PrivateData() const { return private_data_; }
  ssize_t Read(const char* buf, size_t readlen);
  ssize_t Read(const char* buf, size_t readlen) const;
  ssize_t Read(std::string& s);
  ssize_t Read(std::string& s) const;
  ssize_t SyncReadline(std::string& s, long timeout);
  ssize_t SyncReadline(std::string& s, long timeout) const;
  ssize_t SyncRead(const char* buffer, size_t readlen, long timeout);
  ssize_t SyncRead(const char* buffer, size_t readlen, long timeout) const;
  ssize_t SyncRead(std::string& s, long timeout);
  ssize_t SyncRead(std::string& s, long timeout) const;
  ssize_t Write(const char* buffer, size_t len);
  ssize_t Write(const char* buffer, size_t len) const;
  ssize_t Writev(const std::vector<std::pair<char*, size_t>>& mem_blocks);
  ssize_t Writev(const std::vector<std::pair<char*, size_t>>& mem_blocks) const;
  ssize_t SyncWrite(const char* buffer, size_t len, long timeout);
  ssize_t SyncWrite(const char* buffer, size_t len, long timeout) const;
  void Close() { close(fd_); }
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
  mutable ConnState state_;
  std::any private_data_;
  std::weak_ptr<ae::AeEventLoop> el_;
  std::unique_ptr<ConnHandler> read_handler_;
  std::unique_ptr<ConnHandler> write_handler_;
  std::unique_ptr<ConnHandler> accept_handler_;
};
}  // namespace connection
}  // namespace redis_simple
