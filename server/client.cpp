#include "client.h"

#include <vector>

#include "server.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace {
static std::string getCmdName(const std::vector<std::string>& args) {
  std::string name = std::move(args[0]);
  utils::ToUppercase(name);
  return name;
}
}  // namespace

Client::Client(connection::Connection* connection)
    : conn_(connection),
      db_(Server::Get()->DB()),
      cmd_(),
      buf_(std::make_unique<in_memory::ReplyBuffer>()),
      query_buf_(std::make_unique<in_memory::DynamicBuffer>()) {}

ssize_t Client::ReadQuery() {
  char buf[4096];
  memset(buf, 0, sizeof buf);
  ssize_t nread = conn_->Read(buf, 4096);
  printf("nread %zd, buf %s end\n", nread, buf);
  if (nread < 0) {
    return nread;
  }
  query_buf_->WriteToBuffer(buf, nread);
  return nread;
}

ssize_t Client::SendReply() {
  return buf_->ReplyHead() ? SendListReply() : SendBufferReply();
}

ssize_t Client::SendBufferReply() {
  printf("_sendReply %s %zu\n", buf_->UnsentBuffer(),
         buf_->UnsentBufferLength());
  ssize_t nwritten =
      conn_->Write(buf_->UnsentBuffer(), buf_->UnsentBufferLength());
  if (nwritten < 0) {
    return -1;
  }
  buf_->ClearProcessed(nwritten);
  return nwritten;
}

ssize_t Client::SendListReply() {
  printf("_sendvReply\n");
  const std::vector<std::pair<char*, size_t>>& memToWrite = buf_->Memvec();
  ssize_t nwritten = conn_->Writev(memToWrite);
  if (nwritten < 0) {
    return -1;
  }
  buf_->ClearProcessed(nwritten);
  return nwritten;
}

ClientStatus Client::ProcessInputBuffer() {
  while (query_buf_->ProcessedOffset() < query_buf_->NRead()) {
    printf("process loop %zu %zu\n", query_buf_->ProcessedOffset(),
           query_buf_->NRead());
    if (ProcessInlineBuffer() == ClientStatus::clientErr) {
      break;
    }
    if (ProcessCommand() == ClientStatus::clientErr) {
      return ClientStatus::clientErr;
    }
  }
  query_buf_->TrimProcessedBuffer();
  return ClientStatus::clientOK;
}

ClientStatus Client::ProcessInlineBuffer() {
  const std::string& cmdstr = query_buf_->ProcessInlineBuffer();
  if (cmdstr.length() == 0) {
    return ClientStatus::clientErr;
  }
  printf("cmd str %s\n", cmdstr.c_str());
  std::vector<std::string> args = utils::Split(cmdstr, " ");
  if (args.size() == 0) {
    return ClientStatus::clientErr;
  }
  const std::string& name = getCmdName(args);
  args.erase(args.begin());
  std::weak_ptr<const command::Command> cmdptr = command::Command::Create(name);
  if (std::shared_ptr<const command::Command> cmd = cmdptr.lock()) {
    if (!cmd) {
      printf("command not found\n");
      return ClientStatus::clientErr;
    }
  } else {
    printf("command resource expired");
    return ClientStatus::clientErr;
  }
  SetCmd(cmdptr);
  SetCmdArgs(args);
  return ClientStatus::clientOK;
}

ClientStatus Client::ProcessCommand() {
  if (std::shared_ptr<const command::Command> command = cmd_.lock()) {
    printf("process command: %s\n", command->Name().c_str());
    if (command) {
      command->Exec(this);
    }
    return ClientStatus::clientOK;
  } else {
    return ClientStatus::clientErr;
  }
}
}  // namespace redis_simple
