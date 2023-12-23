#include "cli.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <future>

#include "resp_parser.h"
#include "tcp/tcp.h"

namespace redis_simple {
namespace cli {
namespace {
static std::string ReadFromSocket(int fd) {
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

static ssize_t WriteToSocket(int fd, const std::string& cmds) {
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

const std::string& RedisCli::ErrResp = "error";
const std::string& RedisCli::NoReplyResp = "no_reply";

RedisCli::RedisCli()
    : query_buf_(std::make_unique<in_memory::DynamicBuffer>()),
      reply_buf_(std::make_unique<in_memory::DynamicBuffer>()){};

CliStatus RedisCli::Connect(const std::string& ip, const int port) {
  socket_fd_ = tcp::TCP_Connect(ip, port);
  if (socket_fd_ < 0) {
    return CliStatus::cliErr;
  }
  return CliStatus::cliOK;
}

void RedisCli::AddCommand(const std::string& cmd) {
  query_buf_->WriteToBuffer(cmd.c_str(), cmd.size());
}

std::string RedisCli::GetReply() {
  std::string reply;
  if (reply_buf_ && !reply_buf_->Empty() && ProcessReply(reply)) {
    return reply;
  }
  if (!query_buf_->Empty()) {
    if (WriteToSocket(socket_fd_, query_buf_->GetBufInString()) < 0) {
      return ErrResp;
    }
    query_buf_->Clear();
    const std::string& replies = ReadFromSocket(socket_fd_);
    reply_buf_->WriteToBuffer(replies.c_str(), replies.size());
    if (ProcessReply(reply)) {
      return reply;
    }
  }
  return NoReplyResp;
}

CompletableFuture<std::string> RedisCli::GetReplyAsync() {
  std::future<std::string> future =
      std::async(std::launch::async, [&]() { return GetReply(); });
  return CompletableFuture<std::string>(std::move(future));
}

bool RedisCli::ProcessReply(std::string& reply) {
  ssize_t processed = resp_parser::Parse(reply_buf_->GetBufInString(), reply);
  if (processed > 0) {
    reply_buf_->IncrProcessedOffset(processed);
    reply_buf_->TrimProcessedBuffer();
    return true;
  }
  return false;
}
}  // namespace cli
}  // namespace redis_simple

int main() {
  redis_simple::cli::RedisCli cli;
  cli.Connect("localhost", 8081);
  int i = 0;
  while (i++ < 1000) {
    const std::string& cmd1 = "SET key val\r\n";
    cli.AddCommand(cmd1);
    const std::string& cmd2 = "GET key\r\n";
    cli.AddCommand(cmd2);

    const std::string& cmd3 = "ZADD key1 ele1 1.0\r\n";
    const std::string& cmd4 = "ZADD key1 ele2 1.0\r\n";
    cli.AddCommand(cmd3);
    cli.AddCommand(cmd4);

    const std::string& cmd5 = "ZRANK key1 ele1\r\n";
    const std::string& cmd6 = "ZRANK key1 ele2\r\n";
    const std::string& cmd7 = "ZRANK key1 ele3\r\n";
    cli.AddCommand(cmd5);
    cli.AddCommand(cmd6);
    cli.AddCommand(cmd7);

    // const std::string& r1 = cli.GetReply();
    // printf("receive resp %s\n", r1.c_str());
    // const std::string& r2 = cli.GetReply();
    // printf("receive resp %s\n", r2.c_str());

    auto r3 = cli.GetReplyAsync();
    const std::string& applied_str1 =
        r3.ThenApply([](const std::string& reply) {
            printf("receive resp 1: %s end\n", reply.c_str());
            return reply;
          })
            .ThenApply(
                [](const std::string& reply) { return reply + "_processed"; })
            .Get();
    printf("after processed 1, %s\n", applied_str1.c_str());

    auto r4 = cli.GetReplyAsync();
    const std::string& applied_str2 =
        r4.ThenApplyAsync([](const std::string& reply) {
            printf("receive resp 2: %s end\n", reply.c_str());
            return reply;
          })
            .ThenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .Get();
    printf("after processed 2, %s\n", applied_str2.c_str());

    auto r5 = cli.GetReplyAsync();
    const std::string& applied_str3 =
        r5.ThenApplyAsync([](const std::string& reply) {
            printf("receive resp 3: %s end\n", reply.c_str());
            return reply;
          })
            .ThenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .Get();
    printf("after processed 3, %s\n", applied_str3.c_str());

    auto r6 = cli.GetReplyAsync();
    const std::string& applied_str4 =
        r6.ThenApplyAsync([](const std::string& reply) {
            printf("receive resp 4: %s end\n", reply.c_str());
            return reply;
          })
            .ThenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .Get();
    printf("after processed 4, %s\n", applied_str4.c_str());

    auto r7 = cli.GetReplyAsync();
    const std::string& applied_str5 =
        r7.ThenApplyAsync([](const std::string& reply) {
            printf("receive resp 5: %s end\n", reply.c_str());
            return reply;
          })
            .ThenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .Get();
    printf("after processed 5, %s\n", applied_str5.c_str());

    auto r8 = cli.GetReplyAsync();
    const std::string& applied_str6 =
        r8.ThenApplyAsync([](const std::string& reply) {
            printf("receive resp 6: %s end\n", reply.c_str());
            return reply;
          })
            .ThenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .Get();
    printf("after processed 6, %s\n", applied_str6.c_str());

    auto r9 = cli.GetReplyAsync();
    const std::string& applied_str7 =
        r9.ThenApplyAsync([](const std::string& reply) {
            printf("receive resp 7: %s end\n", reply.c_str());
            return reply;
          })
            .ThenApplyAsync(
                [](const std::string& reply) { return reply + "_processed"; })
            .Get();
    printf("after processed 7, %s\n", applied_str7.c_str());
  }

  const std::string& cmd1 = "ZREM key1 ele1\r\n";
  cli.AddCommand(cmd1);
  cli.AddCommand(cmd1);
  const std::string& cmd2 = "ZRANK key1 ele1\r\n";
  cli.AddCommand(cmd2);

  auto r6 = cli.GetReplyAsync();
  const std::string& applied_str4 =
      r6.ThenApplyAsync([](const std::string& reply) {
          printf("receive resp 4: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed 6, %s\n", applied_str4.c_str());
  cli.AddCommand(cmd1);

  auto r7 = cli.GetReplyAsync();
  const std::string& applied_str5 =
      r7.ThenApplyAsync([](const std::string& reply) {
          printf("receive resp 5: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed 7, %s\n", applied_str5.c_str());

  auto r8 = cli.GetReplyAsync();
  const std::string& applied_str6 =
      r8.ThenApplyAsync([](const std::string& reply) {
          printf("receive resp 6: %s end\n", reply.c_str());
          return reply;
        })
          .ThenApplyAsync(
              [](const std::string& reply) { return reply + "_processed"; })
          .Get();
  printf("after processed 8, %s\n", applied_str6.c_str());
}
