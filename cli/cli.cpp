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
  printf("client write %s\n", cmds.c_str());
  while ((nwrite = write(fd, cmds.c_str(), cmds.size())) < cmds.size()) {
    if (nwrite < 0) {
      break;
    }
    nwritten += nwrite;
  }
  return nwritten;
}

static std::string ReplyListToString(const std::vector<std::string>& reply) {
  std::string reply_str;
  for (const std::string& r : reply) {
    reply_str.append(r).push_back('\n');
  }
  return reply_str;
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
  std::vector<std::string> reply;
  {
    const std::lock_guard<std::mutex> lock(lock_);
    if (reply_buf_ && !reply_buf_->Empty()) {
      if (!ProcessReply(reply)) {
        return NoReplyResp;
      }
      const std::string& s = ReplyListToString(reply);
      return ReplyListToString(reply);
    }
  }
  const std::lock_guard<std::mutex> lock(lock_);
  if (!query_buf_->Empty()) {
    if (WriteToSocket(socket_fd_, query_buf_->GetBufInString()) < 0) {
      return ErrResp;
    }
    query_buf_->Clear();
    const std::string& replies = ReadFromSocket(socket_fd_);
    reply_buf_->WriteToBuffer(replies.c_str(), replies.size());
    if (ProcessReply(reply)) {
      const std::string& s = ReplyListToString(reply);
      return ReplyListToString(reply);
    }
  }
  return NoReplyResp;
}

CompletableFuture<std::string> RedisCli::GetReplyAsync() {
  std::future<std::string> future =
      std::async(std::launch::async, [&]() { return GetReply(); });
  return CompletableFuture<std::string>(std::move(future));
}

bool RedisCli::ProcessReply(std::vector<std::string>& reply) {
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
