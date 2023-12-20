#include "client.h"

#include <vector>

#include "server.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace {
std::string getCmdName(const std::vector<std::string>& args) {
  std::string name = std::move(args[0]);
  utils::ToUppercase(name);
  return name;
}
}  // namespace

Client::Client(connection::Connection* connection)
    : conn(connection),
      db(Server::Get()->DB()),
      cmd(),
      buf(std::make_unique<in_memory::ReplyBuffer>()),
      query_buf(std::make_unique<in_memory::DynamicBuffer>()) {}

ssize_t Client::readQuery() {
  char buf[4096];
  memset(buf, 0, sizeof buf);
  ssize_t nread = conn->Read(buf, 4096);
  printf("nread %zd, buf %s end\n", nread, buf);
  if (nread < 0) {
    return nread;
  }
  query_buf->WriteToBuffer(buf, nread);
  return nread;
}

ssize_t Client::sendReply() {
  return buf->ReplyHead() ? _sendvReply() : _sendReply();
}

ssize_t Client::_sendReply() {
  printf("_sendReply %s %zu\n", buf->UnsentBuffer(), buf->UnsentBufferLength());
  ssize_t nwritten =
      conn->Write(buf->UnsentBuffer(), buf->UnsentBufferLength());
  if (nwritten < 0) {
    return -1;
  }
  buf->WriteProcessed(nwritten);
  return nwritten;
}

ssize_t Client::_sendvReply() {
  printf("_sendvReply\n");
  const std::vector<std::pair<char*, size_t>>& memToWrite = buf->Memvec();
  ssize_t nwritten = conn->Writev(memToWrite);
  if (nwritten < 0) {
    return -1;
  }
  buf->WriteProcessed(nwritten);
  return nwritten;
}

ClientStatus Client::processInputBuffer() {
  while (query_buf->ProcessedOffset() < query_buf->NRead()) {
    printf("process loop %zu %zu\n", query_buf->ProcessedOffset(),
           query_buf->NRead());
    if (processInlineBuffer() == ClientStatus::clientErr) {
      break;
    }
    if (processCommand() == ClientStatus::clientErr) {
      return ClientStatus::clientErr;
    }
  }
  query_buf->TrimProcessedBuffer();
  return ClientStatus::clientOK;
}

ClientStatus Client::processInlineBuffer() {
  const std::string& cmdstr = query_buf->ProcessInlineBuffer();
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
  setCmd(cmdptr);
  setArgs(args);
  return ClientStatus::clientOK;
}

ClientStatus Client::processCommand() {
  if (std::shared_ptr<const command::Command> command = cmd.lock()) {
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
