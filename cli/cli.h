#pragma once

#include <unistd.h>

#include <optional>
#include <string>

#include "completable_future.h"
#include "connection/connection.h"
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
  RedisCli(const std::string& ip, const int port);
  CliStatus Connect(const std::string& ip, const int port);
  void AddCommand(const std::string& cmd);
  std::string GetReply();
  CompletableFuture<std::string> GetReplyAsync();
  ~RedisCli() = default;

 private:
  static const std::string& ErrResp;
  static const std::string& NoReplyResp;
  std::string GetReplyAsyncCallback();
  std::optional<std::string> MaybeGetReply();
  std::string GetReplyFromConnection();
  bool ProcessReply(std::vector<std::string>& reply);
  std::shared_ptr<ae::AeEventLoop> el_;
  std::unique_ptr<connection::Connection> connection_;
  std::optional<std::string> ip_;
  std::optional<int> port_;
  std::unique_ptr<in_memory::DynamicBuffer> query_buf_;
  std::unique_ptr<in_memory::DynamicBuffer> reply_buf_;
  std::mutex lock_;
};
}  // namespace cli
}  // namespace redis_simple
