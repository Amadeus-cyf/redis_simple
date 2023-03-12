#include "client.h"

#include <vector>

#include "server.h"
#include "utils/string_utils.h"

namespace redis_simple {
Client::Client()
    : conn(),
      db(Server::get()->getDb()),
      cmd(),
      query_buf(std::make_unique<in_memory::DynamicBuffer>()),
      buf(std::make_unique<in_memory::ReplyBuffer>()) {}

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
  if (nread <= 0) {
    return nread;
  }
  query_buf->writeToBuffer(buf, nread);
  return nread;
}

ssize_t Client::sendReply() {
  return buf->getReplyHead() ? _sendvReply() : _sendReply();
}

ssize_t Client::_sendReply() {
  printf("_sendReply\n");
  const char* buffer = buf->getBuf();
  size_t bufpos = buf->getBufPos();
  size_t sentlen = buf->getSentLen();
  ssize_t nwrite = conn->connWrite(buffer + sentlen, bufpos - sentlen);
  if (nwrite < 0) {
    return nwrite;
  }
  printf("sent reply %zu\n", sentlen + nwrite);
  printf("sent buf %d\n", buffer[0]);
  buf->writeProcessed(nwrite);
  return nwrite;
}

ssize_t Client::_sendvReply() {
  printf("_sendvReply\n");
  std::vector<std::pair<char*, size_t>> memToWrite = buf->getMemvec();
  ssize_t nwritten = conn->connWritev(memToWrite);
  printf("_sendvReply: %zu\n", nwritten);
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
  printf("trim processed query buffer\n");
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
  for (int i = 0; i < args.size(); i++) {
    printf("%s\n", args[i].c_str());
  }
  std::string name = move(args[0]);
  utils::touppercase(name);
  args.erase(args.begin());
  setCmd(new RedisCommand(name, args, t_cmd::getRedisCmdProc(name)));
  return ClientStatus::clientOK;
}

ClientStatus Client::processCommand() {
  printf("process command: %s\n", cmd->toString().c_str());
  ClientStatus status;
  if (cmd) {
    status =
        cmd->exec(this) == 0 ? ClientStatus::clientOK : ClientStatus::clientErr;
  }
  return status;
}
}  // namespace redis_simple