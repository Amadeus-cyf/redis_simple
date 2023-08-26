#include "cli.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <future>

#include "resp_parser.h"
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
  printf("write %s\n", cmds.c_str());
  while ((nwrite = write(fd, cmds.c_str(), cmds.size())) < cmds.size()) {
    if (nwrite < 0) {
      break;
    }
    nwritten += nwrite;
  }
  return nwritten;
}
}  // namespace

const std::string RedisCli::ErrResp = "error";
const std::string RedisCli::NoReplyResp = "no_reply";

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

void RedisCli::addCommand(const std::string& cmd) {
  query_buf->writeToBuffer(cmd.c_str(), cmd.size());
}

std::string RedisCli::getReply() {
  std::string reply;
  if (reply_buf && !reply_buf->isEmpty() && processReply(reply)) {
    return reply;
  }
  if (!query_buf->isEmpty()) {
    if (writeToSocket(socket_fd, query_buf->getBufInString()) < 0) {
      return ErrResp;
    }
    query_buf->clear();
    const std::string& replies = readFromSocket(socket_fd);
    reply_buf->writeToBuffer(replies.c_str(), replies.size());
    if (processReply(reply)) {
      return reply;
    }
  }
  return NoReplyResp;
}

CompletableFuture<std::string> RedisCli::getReplyAsync() {
  std::future<std::string> future =
      std::async(std::launch::async, [&]() { return getReply(); });
  return CompletableFuture<std::string>(std::move(future));
}

bool RedisCli::processReply(std::string& reply) {
  ssize_t processed = resp_parser::parse(reply_buf->getBufInString(), reply);
  if (processed > 0) {
    reply_buf->incrProcessedOffset(processed);
    reply_buf->trimProcessedBuffer();
    return true;
  }
  return false;
}
}  // namespace cli
}  // namespace redis_simple

int main() {
  redis_simple::cli::RedisCli cli;
  cli.connect("localhost", 8081);
  int i = 0;
  while (i++ < 1000) {
    const std::string& cmd1 = "SET key val\r\n";
    cli.addCommand(cmd1);
    const std::string& cmd2 = "GET key\r\n";
    cli.addCommand(cmd2);

    const std::string& cmd3 = "ZADD key1 ele1 1.0\r\n";
    const std::string& cmd4 = "ZADD key1 ele2 1.0\r\n";
    cli.addCommand(cmd3);
    cli.addCommand(cmd4);

    const std::string& cmd5 = "ZRANK key1 ele1\r\n";
    const std::string& cmd6 = "ZRANK key1 ele2\r\n";
    const std::string& cmd7 = "ZRANK key1 ele3\r\n";
    cli.addCommand(cmd5);
    cli.addCommand(cmd6);
    cli.addCommand(cmd7);

    // const std::string& r1 = cli.getReply();
    // printf("receive resp %s\n", r1.c_str());
    // const std::string& r2 = cli.getReply();
    // printf("receive resp %s\n", r2.c_str());
    // printf("test\n");

    auto r3 = cli.getReplyAsync();
    const std::string& applied_str1 =
        r3.thenApply([](const std::string& reply) {
            printf("receive resp 1: %s end\n", reply.c_str());
            return reply;
          })
            .thenApply(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 1, %s\n", applied_str1.c_str());

    auto r4 = cli.getReplyAsync();
    const std::string& applied_str2 =
        r4.thenApplyAsync([](const std::string& reply) {
            printf("receive resp 2: %s end\n", reply.c_str());
            return reply;
          })
            .thenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 2, %s\n", applied_str2.c_str());

    auto r5 = cli.getReplyAsync();
    const std::string& applied_str3 =
        r5.thenApplyAsync([](const std::string& reply) {
            printf("receive resp 3: %s end\n", reply.c_str());
            return reply;
          })
            .thenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 3, %s\n", applied_str3.c_str());

    auto r6 = cli.getReplyAsync();
    const std::string& applied_str4 =
        r6.thenApplyAsync([](const std::string& reply) {
            printf("receive resp 4: %s end\n", reply.c_str());
            return reply;
          })
            .thenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 4, %s\n", applied_str4.c_str());

    auto r7 = cli.getReplyAsync();
    const std::string& applied_str5 =
        r7.thenApplyAsync([](const std::string& reply) {
            printf("receive resp 5: %s end\n", reply.c_str());
            return reply;
          })
            .thenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 5, %s\n", applied_str5.c_str());

    auto r8 = cli.getReplyAsync();
    const std::string& applied_str6 =
        r8.thenApplyAsync([](const std::string& reply) {
            printf("receive resp 6: %s end\n", reply.c_str());
            return reply;
          })
            .thenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 6, %s\n", applied_str6.c_str());

    auto r9 = cli.getReplyAsync();
    const std::string& applied_str7 =
        r9.thenApplyAsync([](const std::string& reply) {
            printf("receive resp 7: %s end\n", reply.c_str());
            return reply;
          })
            .thenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .get();
    printf("after processed 7, %s\n", applied_str7.c_str());
  }

  const std::string& cmd1 = "ZREM key1 ele1\r\n";
  cli.addCommand(cmd1);
  cli.addCommand(cmd1);
  const std::string& cmd2 = "ZRANK key1 ele1\r\n";
  cli.addCommand(cmd2);

  auto r6 = cli.getReplyAsync();
  const std::string& applied_str4 =
      r6.thenApplyAsync([](const std::string& reply) {
          printf("receive resp 4: %s end\n", reply.c_str());
          return reply;
        })
          .thenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .get();
  printf("after processed 6, %s\n", applied_str4.c_str());
  cli.addCommand(cmd1);

  auto r7 = cli.getReplyAsync();
  const std::string& applied_str5 =
      r7.thenApplyAsync([](const std::string& reply) {
          printf("receive resp 5: %s end\n", reply.c_str());
          return reply;
        })
          .thenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .get();
  printf("after processed 7, %s\n", applied_str5.c_str());

  auto r8 = cli.getReplyAsync();
  const std::string& applied_str6 =
      r8.thenApplyAsync([](const std::string& reply) {
          printf("receive resp 6: %s end\n", reply.c_str());
          return reply;
        })
          .thenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .get();
  printf("after processed 8, %s\n", applied_str6.c_str());
}
