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

Client::Client(connection::Connection* connection)
    : connection_(connection),
      db_(Server::Get()->DB()),

      buf_(std::make_unique<in_memory::ReplyBuffer>()),
      query_buf_(std::make_unique<in_memory::DynamicBuffer>()) {}

ssize_t Client::ReadQuery() {
  char buf[4096];
  ssize_t nread = connection_->Read(buf, 4096);
  if (nread <= 0) {
    RS_LOG_DEBUG("nread %zd\n", nread);
    return nread;
  }
  RS_LOG_DEBUG("nread %zd, buf %.*s end\n", nread, static_cast<int>(nread),
               buf);
  query_buf_->WriteToBuffer(buf, nread);
  return nread;
}

ssize_t Client::SendReply() {
  return (buf_->ReplyHead() != nullptr) ? SendListReply() : SendBufferReply();
}

ssize_t Client::SendBufferReply() {
  RS_LOG_DEBUG("_sendReply %s %zu\n", buf_->UnsentBuffer(),
               buf_->UnsentBufferLength());
  ssize_t nwritten =
      connection_->Write(buf_->UnsentBuffer(), buf_->UnsentBufferLength());
  if (nwritten < 0) {
    return -1;
  }
  buf_->ClearProcessed(nwritten);
  return nwritten;
}

ssize_t Client::SendListReply() {
  RS_LOG_DEBUG("_sendvReply\n");
  const auto mem_to_write = buf_->Memvec();
  ssize_t nwritten = connection_->Writev(mem_to_write);
  if (nwritten < 0) {
    return -1;
  }
  buf_->ClearProcessed(nwritten);
  return nwritten;
}

ClientStatus Client::ProcessInputBuffer() {
  while (query_buf_->ProcessedOffset() < query_buf_->NRead()) {
    RS_LOG_DEBUG("process loop %zu %zu\n", query_buf_->ProcessedOffset(),
                 query_buf_->NRead());
    if (ProcessInlineBuffer() == ClientStatus::kError) {
      break;
    }
    if (ProcessCommand() == ClientStatus::kError) {
      return ClientStatus::kError;
    }
  }
  query_buf_->TrimProcessedBuffer();
  return ClientStatus::kOk;
}

ClientStatus Client::ProcessInlineBuffer() {
  const std::string& cmdstr = query_buf_->ProcessInlineBuffer();
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
  auto cmdptr = command::Command::Create(name);
  if (auto cmd = cmdptr.lock()) {
    if (!cmd) {
      RS_LOG_DEBUG("command not found\n");
      return ClientStatus::kError;
    }
  } else {
    RS_LOG_DEBUG("command pointer expired\n");
    return ClientStatus::kError;
  }
  SetCmd(cmdptr);
  SetCmdArgs(args);
  return ClientStatus::kOk;
}

ClientStatus Client::ProcessCommand() {
  if (auto command = cmd_.lock()) {
    RS_LOG_DEBUG("process command: %s\n", command->Name().c_str());
    if (command) {
      command->Exec(this);
    }
    return ClientStatus::kOk;
  }
  return ClientStatus::kError;
}
}  // namespace redis_simple
