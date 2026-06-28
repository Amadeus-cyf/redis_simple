#pragma once

#include <memory>
#include <string>
#include <utility>

#include "connection/connection.h"
#include "memory/dynamic_buffer.h"
#include "memory/reply_buffer.h"
#include "server/client_connection/client_connection.h"
#include "server/commands/command.h"
#include "server/db/db.h"

namespace redis_simple {
enum class ClientStatus {
  kOk = 0,
  kError = -1,
};

class Client {
 public:
  static std::unique_ptr<Client> Create(
      std::unique_ptr<connection::Connection> connection) {
    return std::unique_ptr<Client>(new Client(std::move(connection)));
  }
  int Flags() { return flags_; }
  connection::Connection* Connection() { return connection_.get(); }
  db::RedisDb* Db() { return db_; }
  ssize_t ReadQuery();
  ssize_t SendReply();
  size_t AddReply(const std::string& s) {
    return reply_buf_.Append(s.c_str(), s.length());
  }
  bool HasPendingReplies() { return !(reply_buf_.Empty()); }
  ClientStatus ProcessInputBuffer();
  void Free() { connection_->Close(); }
  const std::vector<std::string>& Args() { return args_; }

 private:
  explicit Client(std::unique_ptr<connection::Connection> connection);
  ClientStatus ParseLine();
  ClientStatus ProcessCommand();
  void SetCmd(const command::Command* command) { command_ = command; }
  void SetArgs(const std::vector<std::string>& args) { args_ = args; }
  ssize_t SendBufferReply();
  ssize_t SendListReply();
  int flags_{};
  db::RedisDb* db_{nullptr};
  const command::Command* command_{nullptr};
  std::vector<std::string> args_;
  std::unique_ptr<connection::Connection> connection_;
  in_memory::DynamicBuffer query_buf_;
  in_memory::ReplyBuffer reply_buf_;
};
}  // namespace redis_simple
