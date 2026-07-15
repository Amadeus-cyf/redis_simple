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
#include "server/reply/reply.h"
namespace redis_simple {
Client::Client(std::unique_ptr<connection::Connection> connection)
    : connection_(std::move(connection)), db_(Server::Get()->Db()) {}

ssize_t Client::ReadQuery() {
  std::array<char, 4096> buf{};
  ssize_t nread = connection_->Read(buf.data(), buf.size());
  if (nread <= 0) {
    RS_LOG_DEBUG("nread %zd\n", nread);
    return nread;
  }
  RS_LOG_DEBUG("nread %zd, buf %.*s end\n", nread, static_cast<int>(nread),
               buf.data());
  query_buf_.Append(buf.data(), static_cast<size_t>(nread));
  return nread;
}

ssize_t Client::SendReply() {
  return (reply_buf_.ReplyHead() != nullptr) ? SendListReply()
                                             : SendBufferReply();
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
  const auto mem_to_write = reply_buf_.Blocks();
  ssize_t nwritten = connection_->WriteVector(mem_to_write);
  if (nwritten < 0) {
    return -1;
  }
  reply_buf_.Consume(nwritten);
  return nwritten;
}

ClientStatus Client::ProcessInputBuffer() {
  while (query_buf_.Consumed() < query_buf_.Size()) {
    RS_LOG_DEBUG("process loop %zu %zu\n", query_buf_.Consumed(),
                 query_buf_.Size());
    const LineStatus status = ParseLine();
    if (status == LineStatus::kIncomplete) {
      break;
    }
    if (status == LineStatus::kRejected) {
      continue;
    }
    if (ProcessCommand() == ClientStatus::kError) {
      return ClientStatus::kError;
    }
  }
  query_buf_.Compact();
  return ClientStatus::kOk;
}

Client::LineStatus Client::ParseLine() {
  command_ = nullptr;
  args_.clear();
  const auto line = query_buf_.ReadLineView();
  if (!line.has_value()) {
    return LineStatus::kIncomplete;
  }
  if (line->empty()) {
    AddReply(reply::FromError("ERR empty command"));
    return LineStatus::kRejected;
  }
  RS_LOG_DEBUG("cmd str %.*s\n", static_cast<int>(line->size()), line->data());
  const size_t command_start = line->find_first_not_of(' ');
  if (command_start == std::string_view::npos) {
    AddReply(reply::FromError("ERR empty command"));
    return LineStatus::kRejected;
  }
  const size_t command_end = line->find(' ', command_start);
  const std::string_view name =
      line->substr(command_start, command_end == std::string_view::npos
                                      ? std::string_view::npos
                                      : command_end - command_start);
  size_t position = command_end;
  while (position != std::string_view::npos) {
    const size_t argument_start = line->find_first_not_of(' ', position);
    if (argument_start == std::string_view::npos) {
      break;
    }
    const size_t argument_end = line->find(' ', argument_start);
    args_.push_back(
        line->substr(argument_start, argument_end == std::string_view::npos
                                         ? std::string_view::npos
                                         : argument_end - argument_start));
    position = argument_end;
  }
  const auto* command = command::Find(name);
  if (command == nullptr) {
    RS_LOG_DEBUG("command not found\n");
    AddReply(reply::UnknownCommand(name));
    return LineStatus::kRejected;
  }
  command_ = command;
  return LineStatus::kReady;
}

ClientStatus Client::ProcessCommand() {
  if (command_ == nullptr) {
    return ClientStatus::kError;
  }
  RS_LOG_DEBUG("process command: %s\n", command_->name);
  command_->callback(this);
  return ClientStatus::kOk;
}
}  // namespace redis_simple
