#include "cli.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <future>

#include "server/connection/tcp.h"

namespace redis_simple {
namespace cli {
namespace {
std::string readFromSocket(int fd) {
  std::string reply;
  ssize_t nread = 0;
  char buf[4096];
  while ((nread = read(fd, buf, 4096)) != EOF) {
    if (nread == 0) {
      break;
    }
    reply.append(buf, nread);
    memset(buf, 0, sizeof(buf));
    if (reply.size() > 0 && reply[reply.size() - 2] == '\r' &&
        reply[reply.size() - 1] == '\n') {
      break;
    }
  }
  return reply;
}

ssize_t writeToSocket(int fd, const std::string& cmds) {
  ssize_t nwritten = 0;
  int nwrite = 0;
  while ((nwrite = write(fd, cmds.c_str(), cmds.size())) < cmds.size()) {
    if (nwrite < 0) {
      break;
    }
    nwritten += nwrite;
  }
  return nwritten;
}
}  // namespace

const std::string RedisCli::ErrResp = "+error";
const std::string RedisCli::InProgressResp = "+inprogress";
const std::string RedisCli::NoReplyResp = "+no_reply";

RedisCli::RedisCli()
    : query_buf(std::make_unique<in_memory::QueryBuffer>()),
      reply_buf(std::make_unique<in_memory::QueryBuffer>()){};

CliStatus RedisCli::connect(const std::string& ip, const int port,
                            const bool nonBlock) {
  non_block = nonBlock;
  socket_fd = tcp::tcpConnect(ip, port, non_block);
  if (socket_fd < 0) {
    return CliStatus::cliErr;
  }
  return CliStatus::cliOK;
}

void RedisCli::addCommand(const std::string& cmd, const size_t len) {
  query_buf->writeToBuffer(cmd.c_str(), len);
}

std::string RedisCli::getReply() {
  if (reply_buf && !reply_buf->isEmpty()) {
    const std::string& reply = reply_buf->processInlineBuffer();
    reply_buf->trimProcessedBuffer();
    return reply;
  }
  if (!query_buf->isEmpty()) {
    if (writeToSocket(socket_fd, query_buf->getBufInString()) < 0) {
      return ErrResp;
    }
    query_buf->clear();
    const std::string& replies = readFromSocket(socket_fd);
    reply_buf->writeToBuffer(replies.c_str(), replies.size());
    const std::string& reply = reply_buf->processInlineBuffer();
    reply_buf->trimProcessedBuffer();
    return reply;
  }
  return NoReplyResp;
}

ae::AeEventStatus RedisCli::writeToServer(ae::AeEventLoop* el, int fd,
                                          void* clientData, int mask) {
  RedisCli* cli = static_cast<RedisCli*>(clientData);
  if (cli->query_buf->isEmpty()) {
    el->aeDelFileEvent(fd, ae::AeFlags::aeWritable);
  }
  return writeToSocket(cli->socket_fd, cli->query_buf->getBufInString()) >= 0
             ? ae::AeEventStatus::aeEventOK
             : ae::AeEventStatus::aeEventErr;
}

ae::AeEventStatus RedisCli::readFromServer(ae::AeEventLoop* el, int fd,
                                           void* clientData, int mask) {
  RedisCli* cli = static_cast<RedisCli*>(clientData);
  const std::string& reply = readFromSocket(fd);
  if (reply.size() > 0) {
    cli->reply_buf->writeToBuffer(reply.c_str(), reply.size());
  }
  cli->query_buf->clear();
  return ae::AeEventStatus::aeEventOK;
}
}  // namespace cli
}  // namespace redis_simple

int main() {
  int i = 0;
  redis_simple::cli::RedisCli cli;
  cli.connect("localhost", 8081, false);
  while (i++ < 1000) {
    const std::string& cmd1 = "SET key val\r\n";
    cli.addCommand(cmd1, cmd1.size());
    const std::string& cmd2 = "SET key\r\n";
    cli.addCommand(cmd2, cmd2.size());
    const std::string& r1 = cli.getReply();
    printf("receive resp %s\n", r1.c_str());
    const std::string& r2 = cli.getReply();
    printf("receive resp %s\n", r2.c_str());
  }
}
