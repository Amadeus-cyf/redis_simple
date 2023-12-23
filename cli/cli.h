#pragma once

#include <unistd.h>

#include <string>

#include "completable_future.h"
#include "event_loop/ae.h"
#include "memory/dynamic_buffer.h"

namespace redis_simple {
namespace cli {
enum CliStatus {
  cliOK = 1,
  cliErr = -1,
};

class RedisCli {
 public:
  RedisCli();
  CliStatus Connect(const std::string& ip, const int port);
  void AddCommand(const std::string& cmd);
  std::string GetReply();
  CompletableFuture<std::string> GetReplyAsync();
  ~RedisCli() { close(socket_fd_); }

 private:
  static const std::string& ErrResp;
  static const std::string& NoReplyResp;
  bool ProcessReply(std::string& reply);
  int socket_fd_;
  std::string cli_ip_;
  int cli_port_;
  std::unique_ptr<in_memory::DynamicBuffer> query_buf_;
  std::unique_ptr<in_memory::DynamicBuffer> reply_buf_;
};
}  // namespace cli
}  // namespace redis_simple
