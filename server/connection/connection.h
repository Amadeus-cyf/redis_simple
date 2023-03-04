#pragma once

#include <string>

#include "server/conn_handler/conn_handler.h"
#include "server/event_loop/ae.h"

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
};

class Connection {
 public:
  Connection();
  explicit Connection(int cfd);
  void bindEventLoop(ae::AeEventLoop* loop) { el = loop; }
  StatusCode connect(const std::string& remote_ip, int remote_port,
                     const std::string& local_ip, int local_port);
  StatusCode listen(const std::string& ip, int port);
  StatusCode accept(std::string* remote_ip, int* remote_port);
  void setReadHandler(std::unique_ptr<ConnHandler> handler);
  void unsetReadHandler();
  bool hasReadHandler() { return read_handler != nullptr; }
  void setWriteHandler(std::unique_ptr<ConnHandler> handler);
  void unsetWriteHandler();
  bool hasWriteHandler() { return write_handler != nullptr; }
  int getFd() { return fd; }
  ConnState getState() { return state; }
  void setState(ConnState state) { this->state = state; }
  void setPrivateData(void* data) { private_data = data; }
  void* getPrivateData() { return private_data; }
  ssize_t connRead(const char* buf, size_t readlen);
  ssize_t connRead(std::string& s);
  ssize_t connSyncReadline(std::string& s, long timeout);
  ssize_t connSyncRead(const char* buffer, size_t readlen, long timeout);
  ssize_t connSyncRead(std::string& s, long timeout);
  ssize_t connWrite(const char* buffer, size_t len);
  ssize_t connWritev(const std::vector<std::pair<char*, size_t>>& mem_blocks);
  ssize_t connSyncWrite(const char* buffer, size_t len, long timeout);
  bool isBlock();
  // void free();

 private:
  int fd;
  int flags;
  ae::AeEventLoop* el;
  ConnState state;
  std::unique_ptr<ConnHandler> read_handler;
  std::unique_ptr<ConnHandler> write_handler;
  std::unique_ptr<ConnHandler> accept_handler;

  static ae::AeEventStatus connSocketEventHandler(int fd, void* client_data,
                                                  int mask);
  bool boundEventLoop() { return el != nullptr; }
  void* private_data;
};
}  // namespace connection
}  // namespace redis_simple
