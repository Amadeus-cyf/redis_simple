#pragma once

#include <unistd.h>

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
  ae::AeEventLoop* loop;
  int fd;
};

class Connection {
 public:
  explicit Connection(const Context& ctx);
  StatusCode connect(const std::string& remote_ip, int remote_port,
                     const std::string& local_ip, int local_port);
  StatusCode listen(const std::string& ip, int port);
  StatusCode accept(std::string* remote_ip, int* remote_port);
  void setReadHandler(std::unique_ptr<ConnHandler> handler);
  void unsetReadHandler();
  bool hasReadHandler() { return read_handler != nullptr; }
  bool hasReadHandler() const { return read_handler != nullptr; }
  void setWriteHandler(std::unique_ptr<ConnHandler> handler);
  void unsetWriteHandler();
  bool hasWriteHandler() { return write_handler != nullptr; }
  bool hasWriteHandler() const { return write_handler != nullptr; }
  int getFd() { return fd; }
  ConnState getState() { return state; }
  ConnState getState() const { return state; }
  void setState(ConnState _state) { state = _state; }
  void setPrivateData(void* _private_data) { private_data = _private_data; }
  void* getPrivateData() { return private_data; }
  void* getPrivateData() const { return private_data; }
  ssize_t connRead(const char* buf, size_t readlen);
  ssize_t connRead(const char* buf, size_t readlen) const;
  ssize_t connRead(std::string& s);
  ssize_t connRead(std::string& s) const;
  ssize_t connSyncReadline(std::string& s, long timeout);
  ssize_t connSyncReadline(std::string& s, long timeout) const;
  ssize_t connSyncRead(const char* buffer, size_t readlen, long timeout);
  ssize_t connSyncRead(const char* buffer, size_t readlen, long timeout) const;
  ssize_t connSyncRead(std::string& s, long timeout);
  ssize_t connSyncRead(std::string& s, long timeout) const;
  ssize_t connWrite(const char* buffer, size_t len);
  ssize_t connWrite(const char* buffer, size_t len) const;
  ssize_t connWritev(const std::vector<std::pair<char*, size_t>>& mem_blocks);
  ssize_t connWritev(
      const std::vector<std::pair<char*, size_t>>& mem_blocks) const;
  ssize_t connSyncWrite(const char* buffer, size_t len, long timeout);
  ssize_t connSyncWrite(const char* buffer, size_t len, long timeout) const;
  void connClose() { close(fd); }
  ~Connection() { connClose(); }

 private:
  static ae::AeEventStatus connSocketEventHandler(ae::AeEventLoop* el, int fd,
                                                  Connection* client_data,
                                                  int mask);
  int fd;
  int flags;
  ae::AeEventLoop* el;
  mutable ConnState state;
  std::unique_ptr<ConnHandler> read_handler;
  std::unique_ptr<ConnHandler> write_handler;
  std::unique_ptr<ConnHandler> accept_handler;
  void* private_data;
};
}  // namespace connection
}  // namespace redis_simple
