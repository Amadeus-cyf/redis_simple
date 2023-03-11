#pragma once

#include <string>

#include "completable_future.h"
#include "server/event_loop/ae.h"
#include "server/memory/query_buffer.h"

namespace redis_simple {
namespace cli {
enum CliStatus {
  cliOK = 1,
  cliErr = -1,
};

class RedisCli {
 public:
  RedisCli();
  CliStatus connect(const std::string& ip, const int port);
  void addCommand(const std::string& cmd, const size_t len);
  std::string getReply();
  CompletableFuture<std::string> getReplyAsync();

 private:
  static const std::string ErrResp;
  static const std::string InProgressResp;
  static const std::string NoReplyResp;
  int socket_fd;
  std::string cli_ip;
  int cli_port;
  std::unique_ptr<in_memory::QueryBuffer> query_buf;
  std::unique_ptr<in_memory::QueryBuffer> reply_buf;
};
}  // namespace cli
}  // namespace redis_simple
