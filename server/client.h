#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

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
  int Flags() const { return flags_; }
  connection::Connection* Connection() { return connection_.get(); }
  const connection::Connection* Connection() const { return connection_.get(); }
  db::RedisDb* Db() { return db_; }
  ssize_t ReadQuery();
  ssize_t SendReply();
  size_t AddReply(std::string_view reply) {
    return reply_buf_.Append(reply.data(), reply.size());
  }
  bool HasPendingReplies() const { return !reply_buf_.Empty(); }
  ClientStatus ProcessInputBuffer();
  void Free() { connection_->Close(); }
  const command::CommandArgs& Args() const { return args_; }

 private:
  enum class LineStatus {
    kReady,
    kIncomplete,
    kRejected,
  };

  explicit Client(std::unique_ptr<connection::Connection> connection);
  LineStatus ParseLine();
  ClientStatus ProcessCommand();
  ssize_t SendBufferReply();
  ssize_t SendListReply();
  int flags_{};
  std::unique_ptr<connection::Connection> connection_;
  db::RedisDb* db_{nullptr};
  const command::Command* command_{nullptr};
  command::CommandArgs args_;
  in_memory::DynamicBuffer query_buf_;
  in_memory::ReplyBuffer reply_buf_;
};
}  // namespace redis_simple
