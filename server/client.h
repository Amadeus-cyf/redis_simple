#pragma once

#include <sys/types.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "connection/connection.h"
#include "memory/dynamic_buffer.h"
#include "memory/reply_buffer.h"
#include "server/commands/command.h"
#include "server/db/db.h"
#include "server/reply.h"

namespace redis_simple::aof {
class Aof;
}

namespace redis_simple {
enum class ClientStatus {
  kOk = 0,
  kError = -1,
};

struct OutputBufferLimits {
  size_t resume_bytes{size_t{4} * 1024 * 1024};
  size_t soft_bytes{size_t{8} * 1024 * 1024};
  size_t hard_bytes{size_t{64} * 1024 * 1024};
};

class Client {
 public:
  static std::unique_ptr<Client> Create(
      std::unique_ptr<connection::Connection> connection,
      OutputBufferLimits output_limits = {}) {
    return std::unique_ptr<Client>(
        new Client(std::move(connection), output_limits));
  }
  connection::Connection* Connection() { return connection_.get(); }
  const connection::Connection* Connection() const { return connection_.get(); }
  db::RedisDb* Db() { return db_; }
  ssize_t ReadQuery();
  ssize_t SendReply();
  size_t AddReply(std::string_view reply);
  size_t AddReply(std::string&& reply);
  size_t AddReply(std::string&& header, std::string&& body);
  bool HasPendingReplies() const { return !reply_buf_.Empty(); }
  size_t PendingReplyBytes() const { return reply_buf_.PendingBytes(); }
  bool ShouldPauseReads() const;
  bool ShouldResumeReads() const;
  bool ReadsPaused() const { return reads_paused_; }
  void SetReadsPaused(bool paused) { reads_paused_ = paused; }
  bool ShouldCloseAfterReply() const { return close_after_reply_; }
  void CloseAfterReply() { close_after_reply_ = true; }
  reply::ProtocolVersion Protocol() const { return protocol_; }
  void SetProtocol(reply::ProtocolVersion protocol) { protocol_ = protocol; }
  ClientStatus ProcessInputBuffer();
  void Free() { connection_->Close(); }
  const command::CommandArgs& Args() const { return args_; }
  void MarkModified() { modified_ = true; }

 private:
  friend class aof::Aof;
  enum class RequestStatus {
    kReady,
    kIncomplete,
    kRejected,
    kProtocolError,
  };

  Client(std::unique_ptr<connection::Connection> connection,
         OutputBufferLimits output_limits);
  explicit Client(db::RedisDb* db);
  bool ExecuteForReplay(const command::Command* command,
                        command::CommandArgs* args);
  RequestStatus ParseRequest();
  ClientStatus ProcessCommand();
  bool CanQueueReply(size_t size);
  ssize_t SendBufferReply();
  ssize_t SendListReply();
  std::unique_ptr<connection::Connection> connection_;
  db::RedisDb* db_{nullptr};
  aof::Aof* aof_{nullptr};
  const command::Command* command_{nullptr};
  command::CommandArgs args_;
  in_memory::DynamicBuffer query_buf_;
  in_memory::ReplyBuffer reply_buf_;
  std::vector<iovec> reply_blocks_;
  OutputBufferLimits output_limits_;
  reply::ProtocolVersion protocol_{reply::ProtocolVersion::kResp2};
  bool reads_paused_{};
  bool close_after_reply_{};
  bool modified_{};
  bool discard_replies_{};
  bool reply_error_{};
};
}  // namespace redis_simple
