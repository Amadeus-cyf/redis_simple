#include "cli.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <future>

#include "tcp/tcp.h"

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
    : query_buf(std::make_unique<in_memory::DynamicBuffer>()),
      reply_buf(std::make_unique<in_memory::DynamicBuffer>()){};

CliStatus RedisCli::connect(const std::string& ip, const int port) {
  socket_fd = tcp::tcpConnect(ip, port);
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

CompletableFuture<std::string> RedisCli::getReplyAsync() {
  std::future<std::string> future =
      std::async(std::launch::async, [&]() { return getReply(); });
  return CompletableFuture<std::string>(std::move(future));
}
}  // namespace cli
}  // namespace redis_simple

int main() {
  redis_simple::cli::RedisCli cli;
  cli.connect("localhost", 8081);
  int i = 0;
  while (i++ < 1000) {
    const std::string& cmd1 = "SET key val\r\n";
    cli.addCommand(cmd1, cmd1.size());
    const std::string& cmd2 = "SET key\r\n";
    cli.addCommand(cmd2, cmd2.size());

    // const std::string& r1 = cli.getReply();
    // printf("receive resp %s\n", r1.c_str());
    // const std::string& r2 = cli.getReply();
    // printf("receive resp %s\n", r2.c_str());

    auto r3 = cli.getReplyAsync();
    const std::string& applied_str1 =
        r3.thenApply([](const std::string& reply) {
            printf("receive resp 1: %s\n", reply.c_str());
            return reply;
          })
            .thenApply(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 1, %s\n", applied_str1.c_str());

    auto r4 = cli.getReplyAsync();
    const std::string& applied_str2 =
        r4.thenApplyAsync([](const std::string& reply) {
            printf("receive resp 2: %s\n", reply.c_str());
            return reply;
          })
            .thenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 2, %s\n", applied_str2.c_str());
  }
}
