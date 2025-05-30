#pragma once

#include <string>

#include "connection/connection.h"
#include "memory/dynamic_buffer.h"
#include "memory/reply_buffer.h"
#include "server/commands/command.h"
#include "server/db/db.h"
#include "server/networking/networking.h"

namespace redis_simple {
enum class ClientStatus {
  clientOK = 0,
  clientErr = -1,
};

class Client {
 public:
  static Client* Create(connection::Connection* connection) {
    return new Client(connection);
  }
  int Flags() { return flags; }
  connection::Connection* Connection() { return connection_.get(); }
  std::weak_ptr<const db::RedisDb> DB() { return db_; }
  ssize_t ReadQuery();
  ssize_t SendReply();
  size_t AddReply(const std::string& s) {
    return buf_->AddReplyToBufferOrList(s.c_str(), s.length());
  }
  bool HasPendingReplies() { return !(buf_->Empty()); }
  ClientStatus ProcessInputBuffer();
  void Free() { connection_->Close(); }
  void Free() const { connection_->Close(); }
  const std::vector<std::string>& CmdArgs() { return args_; }

 private:
  explicit Client(connection::Connection* connection);
  ClientStatus ProcessInlineBuffer();
  ClientStatus ProcessCommand();
  void SetCmd(std::weak_ptr<const command::Command> command) { cmd_ = command; }
  void SetCmdArgs(const std::vector<std::string>& args) { args_ = args; }
  ssize_t SendBufferReply();
  ssize_t SendListReply();
  // Client flags
  int flags;
  std::weak_ptr<const db::RedisDb> db_;
  // Current command
  std::weak_ptr<const command::Command> cmd_;
  // Current command args
  std::vector<std::string> args_;
  // Connection
  std::unique_ptr<connection::Connection> connection_;
  // In memory buffer used to store incoming query.
  std::unique_ptr<in_memory::DynamicBuffer> query_buf_;
  // In memory buffer used to store reply of command.
  std::unique_ptr<in_memory::ReplyBuffer> buf_;
};
}  // namespace redis_simple
