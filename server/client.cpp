#include "client.h"

#include <vector>

#include "server.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace {
std::string getCmdName(const std::vector<std::string>& args) {
  std::string name = std::move(args[0]);
  utils::touppercase(name);
  return name;
}
}  // namespace

Client::Client(connection::Connection* connection)
    : conn(connection),
      db(Server::get()->getDb()),
      cmd(),
      buf(std::make_unique<in_memory::ReplyBuffer>()),
      query_buf(std::make_unique<in_memory::DynamicBuffer>()) {}

ssize_t Client::readQuery() {
  char buf[4096];
  memset(buf, 0, sizeof buf);
  ssize_t nread = conn->connRead(buf, 4096);
  printf("nread %zd, buf %s end\n", nread, buf);
  if (nread < 0) {
    return nread;
  }
  query_buf->writeToBuffer(buf, nread);
  return nread;
}

ssize_t Client::sendReply() {
  return buf->getReplyHead() ? _sendvReply() : _sendReply();
}

ssize_t Client::_sendReply() {
  printf("_sendReply %s %zu\n", buf->getUnsentBuffer(),
         buf->getUnsentBufferLength());
  ssize_t nwritten =
      conn->connWrite(buf->getUnsentBuffer(), buf->getUnsentBufferLength());
  if (nwritten < 0) {
    return -1;
  }
  buf->writeProcessed(nwritten);
  return nwritten;
}

ssize_t Client::_sendvReply() {
  printf("_sendvReply\n");
  const std::vector<std::pair<char*, size_t>>& memToWrite = buf->getMemvec();
  ssize_t nwritten = conn->connWritev(memToWrite);
  if (nwritten < 0) {
    return -1;
  }
  buf->writeProcessed(nwritten);
  return nwritten;
}

ClientStatus Client::processInputBuffer() {
  while (query_buf->getProcessedOffset() < query_buf->getRead()) {
    printf("process loop %zu %zu\n", query_buf->getProcessedOffset(),
           query_buf->getRead());
    if (processInlineBuffer() == ClientStatus::clientErr) {
      break;
    }
    if (processCommand() == ClientStatus::clientErr) {
      return ClientStatus::clientErr;
    }
  }
  query_buf->trimProcessedBuffer();
  return ClientStatus::clientOK;
}

ClientStatus Client::processInlineBuffer() {
  const std::string& cmdstr = query_buf->processInlineBuffer();
  if (cmdstr.length() == 0) {
    return ClientStatus::clientErr;
  }
  printf("cmd str %s\n", cmdstr.c_str());
  std::vector<std::string> args = utils::split(cmdstr, " ");
  if (args.size() == 0) {
    return ClientStatus::clientErr;
  }
  const std::string& name = getCmdName(args);
  args.erase(args.begin());
  std::weak_ptr<const command::Command> cmdptr = command::Command::create(name);
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
    printf("process command: %s\n", command->getName().c_str());
    if (command) {
      command->exec(this);
    }
    return ClientStatus::clientOK;
  } else {
    return ClientStatus::clientErr;
  }
}
}  // namespace redis_simple
