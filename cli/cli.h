#pragma once

#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "connection/connection.h"
#include "memory/dynamic_buffer.h"

namespace redis_simple::cli {
enum class CliStatus {
  kOk = 1,
  kError = -1,
};

class RedisCli {
 public:
  RedisCli();
  RedisCli(const std::string& ip, int port);
  CliStatus Connect(const std::string& ip, int port);
  void AddCommand(const std::string& cmd);
  std::string GetReply();
  std::future<std::string> GetReplyAsync();
  ~RedisCli() = default;

 private:
  static const std::string& ErrResp;
  static const std::string& NoReplyResp;
  std::optional<std::string> MaybeGetReply();
  std::string GetReplyFromConnection();
  bool ProcessReply(std::vector<std::string>& reply);
  std::unique_ptr<connection::Connection> connection_;
  std::optional<std::string> ip_;
  std::optional<int> port_;
  in_memory::DynamicBuffer query_buf_;
  in_memory::DynamicBuffer reply_buf_;
  std::mutex lock_;
};
}  // namespace redis_simple::cli
