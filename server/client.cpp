#include "client.h"

#include <vector>

#include "server.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace {
std::string GetCmdName(const std::vector<std::string>& args) {
  std::string name = args[0];
  utils::ToUppercase(name);
  return name;
}
}  // namespace

Client::Client(std::unique_ptr<connection::Connection> connection)
    : connection_(std::move(connection)), db_(Server::Get()->Db()) {}

ssize_t Client::ReadQuery() {
  char buf[4096];
  ssize_t nread = connection_->Read(buf, 4096);
  if (nread <= 0) {
    RS_LOG_DEBUG("nread %zd\n", nread);
    return nread;
  }
  RS_LOG_DEBUG("nread %zd, buf %.*s end\n", nread, static_cast<int>(nread),
               buf);
  query_buf_.Append(buf, nread);
  return nread;
}

ssize_t Client::SendReply() {
  return (reply_buf_.ReplyHead() != nullptr) ? SendListReply()
                                             : SendBufferReply();
}

ssize_t Client::SendBufferReply() {
  RS_LOG_DEBUG("_sendReply %s %zu\n", reply_buf_.UnsentBuffer(),
               reply_buf_.UnsentLength());
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
    if (ParseLine() == ClientStatus::kError) {
      break;
    }
    if (ProcessCommand() == ClientStatus::kError) {
      return ClientStatus::kError;
    }
  }
  query_buf_.Compact();
  return ClientStatus::kOk;
}

ClientStatus Client::ParseLine() {
  const std::string& cmdstr = query_buf_.ReadLine();
  if (cmdstr.empty()) {
    return ClientStatus::kError;
  }
  RS_LOG_DEBUG("cmd str %s\n", cmdstr.c_str());
  auto args = utils::Split(cmdstr, " ");
  if (args.empty()) {
    return ClientStatus::kError;
  }
  const std::string& name = GetCmdName(args);
  args.erase(args.begin());
  const auto* command = command::Find(name);
  if (command == nullptr) {
    RS_LOG_DEBUG("command not found\n");
    return ClientStatus::kError;
  }
  SetCmd(command);
  SetArgs(args);
  return ClientStatus::kOk;
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
