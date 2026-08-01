#include "client.h"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "logging/logger.h"
#include "server.h"
#include "server/reply.h"
#include "server/request_parser.h"
namespace redis_simple {
namespace {
constexpr auto kReadBufferSize = size_t{16} * 1024;
constexpr auto kMaxQueryBufferSize = size_t{64} * 1024 * 1024;
}  // namespace

Client::Client(std::unique_ptr<connection::Connection> connection,
               OutputBufferLimits output_limits)
    : connection_(std::move(connection)),
      db_(Server::Get()->Db()),
      output_limits_(output_limits) {}

size_t Client::AddReply(std::string_view reply_text) {
  if (!CanQueueReply(reply_text.size())) {
    return 0;
  }
  return reply_buf_.Append(reply_text.data(), reply_text.size());
}

size_t Client::AddReply(std::string&& reply_text) {
  if (!CanQueueReply(reply_text.size())) {
    return 0;
  }
  return reply_buf_.Append(std::move(reply_text));
}

size_t Client::AddReply(std::string&& header, std::string&& body) {
  if (body.size() > std::numeric_limits<size_t>::max() - header.size()) {
    close_after_reply_ = true;
    return 0;
  }
  const size_t total_size = header.size() + body.size();
  if (!CanQueueReply(total_size)) {
    return 0;
  }
  const size_t header_size = reply_buf_.Append(std::move(header));
  const size_t body_size = reply_buf_.Append(std::move(body));
  return header_size + body_size;
}

bool Client::ShouldPauseReads() const {
  return !reads_paused_ && (close_after_reply_ ||
                            (output_limits_.soft_bytes > 0 &&
                             PendingReplyBytes() >= output_limits_.soft_bytes));
}

bool Client::ShouldResumeReads() const {
  return reads_paused_ && !close_after_reply_ &&
         PendingReplyBytes() <= output_limits_.resume_bytes;
}

bool Client::CanQueueReply(size_t size) {
  const size_t pending = PendingReplyBytes();
  if (size <= output_limits_.hard_bytes &&
      pending <= output_limits_.hard_bytes - size) {
    return true;
  }
  RS_LOG_DEBUG("client output buffer limit exceeded\n");
  close_after_reply_ = true;
  return false;
}

ssize_t Client::ReadQuery() {
  std::array<char, kReadBufferSize> buffer{};
  size_t total_read = 0;
  while (true) {
    const ssize_t nread = connection_->Read(buffer.data(), buffer.size());
    if (nread > 0) {
      const auto chunk_size = static_cast<size_t>(nread);
      if (query_buf_.Size() > kMaxQueryBufferSize ||
          chunk_size > kMaxQueryBufferSize - query_buf_.Size()) {
        RS_LOG_DEBUG("query buffer limit exceeded\n");
        connection_->SetState(connection::ConnectionState::kError);
        return -1;
      }
      query_buf_.Append(buffer.data(), chunk_size);
      total_read += chunk_size;
      continue;
    }
    if (nread == 0) {
      break;
    }
    if (connection_->State() != connection::ConnectionState::kConnected) {
      return -1;
    }
    break;
  }
  RS_LOG_DEBUG("read %zu query bytes\n", total_read);
  return static_cast<ssize_t>(total_read);
}

ssize_t Client::SendReply() {
  return reply_buf_.ReplyCount() > 0 ? SendListReply() : SendBufferReply();
}

ssize_t Client::SendBufferReply() {
  const size_t reply_length = reply_buf_.UnsentLength();
  const int logged_length = static_cast<int>(std::min(
      reply_length, static_cast<size_t>(std::numeric_limits<int>::max())));
  RS_LOG_DEBUG("_sendReply %.*s %zu\n", logged_length,
               reply_buf_.UnsentBuffer(), reply_length);
  ssize_t nwritten =
      connection_->Write(reply_buf_.UnsentBuffer(), reply_buf_.UnsentLength());
  if (nwritten < 0) {
    return -1;
  }
  reply_buf_.Consume(nwritten);
  return nwritten;
}

ssize_t Client::SendListReply() {
  RS_LOG_DEBUG("_sendvReply\n");
  reply_buf_.FillBlocks(&reply_blocks_);
  ssize_t nwritten = connection_->WriteVector(reply_blocks_);
  if (nwritten < 0) {
    return -1;
  }
  reply_buf_.Consume(nwritten);
  return nwritten;
}

ClientStatus Client::ProcessInputBuffer() {
  while (!close_after_reply_ && query_buf_.Consumed() < query_buf_.Size()) {
    RS_LOG_DEBUG("process loop %zu %zu\n", query_buf_.Consumed(),
                 query_buf_.Size());
    const RequestStatus status = ParseRequest();
    if (status == RequestStatus::kIncomplete ||
        status == RequestStatus::kProtocolError) {
      break;
    }
    if (status == RequestStatus::kRejected) {
      continue;
    }
    if (ProcessCommand() == ClientStatus::kError) {
      return ClientStatus::kError;
    }
  }
  query_buf_.Compact();
  return ClientStatus::kOk;
}

Client::RequestStatus Client::ParseRequest() {
  command_ = nullptr;
  args_.clear();
  std::string_view name;
  const auto parsed = request_parser::Parse(query_buf_.View(), &name, &args_);
  if (parsed.status == request_parser::ParseStatus::kIncomplete) {
    return RequestStatus::kIncomplete;
  }
  if (parsed.status == request_parser::ParseStatus::kInvalid) {
    AddReply(reply::FromError("ERR Protocol error: invalid request"));
    close_after_reply_ = true;
    query_buf_.Clear();
    return RequestStatus::kProtocolError;
  }
  query_buf_.Consume(parsed.consumed);
  if (name.empty()) {
    AddReply(reply::FromError("ERR empty command"));
    return RequestStatus::kRejected;
  }
  RS_LOG_DEBUG("command name %.*s\n", static_cast<int>(name.size()),
               name.data());
  const auto* command = command::Find(name);
  if (command == nullptr) {
    RS_LOG_DEBUG("command not found\n");
    AddReply(reply::UnknownCommand(name));
    return RequestStatus::kRejected;
  }
  if (!command->arity.Accepts(args_.size())) {
    AddReply(reply::WrongNumberOfArguments());
    return RequestStatus::kRejected;
  }
  command_ = command;
  return RequestStatus::kReady;
}

ClientStatus Client::ProcessCommand() {
  if (command_ == nullptr) {
    return ClientStatus::kError;
  }
  RS_LOG_DEBUG("process command: %.*s\n",
               static_cast<int>(command_->name.size()), command_->name.data());
  command_->callback(this);
  return ClientStatus::kOk;
}
}  // namespace redis_simple
