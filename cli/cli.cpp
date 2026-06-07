#include "cli.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <future>

#include "resp_parser.h"
#include "tcp/tcp.h"

namespace redis_simple {
namespace cli {
namespace {
std::string ReadFromConnection(const connection::Connection* connection) {
  std::string reply;
  ssize_t nread = 0;
  char buf[4096];
  while ((nread = connection->SyncRead(buf, 4096, 1000)) != EOF) {
    if (nread == 0) {
      break;
    }
    reply.append(buf, nread);
    if (reply.size() >= 2 && reply[reply.size() - 2] == '\r' &&
        reply[reply.size() - 1] == '\n') {
      break;
    }
  }
  return reply;
}

ssize_t WriteToConnection(const connection::Connection* connection,
                          const std::string& cmds) {
  ssize_t nwritten = 0;
  RS_LOG_DEBUG("client write %s\n", cmds.c_str());
  while (nwritten < cmds.size()) {
    const auto remaining = cmds.size() - nwritten;
    ssize_t n = connection->SyncWrite(cmds.c_str() + nwritten, remaining, 1000);
    if (n <= 0) {
      break;
    }
    nwritten += n;
  }
  return nwritten;
}

std::string ReplyListToString(const std::vector<std::string>& reply) {
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
    : ip_(std::nullopt),
      port_(std::nullopt),
      query_buf_(std::make_unique<in_memory::DynamicBuffer>()),
      reply_buf_(std::make_unique<in_memory::DynamicBuffer>()){};

RedisCli::RedisCli(const std::string& ip, const int port)
    : ip_(ip),
      port_(port),
      query_buf_(std::make_unique<in_memory::DynamicBuffer>()),
      reply_buf_(std::make_unique<in_memory::DynamicBuffer>()){};

CliStatus RedisCli::Connect(const std::string& ip, const int port) {
  connection::Context ctx;
  ctx.event_loop = std::shared_ptr<ae::EventLoop>();
  ctx.fd = -1;
  connection_ = std::make_unique<connection::Connection>(ctx);
  const connection::AddressInfo remote(ip, port);
  std::optional<connection::AddressInfo> local;
  if (ip_.has_value() && port_.has_value()) {
    local.emplace(connection::AddressInfo(*ip_, *port_));
  }
  connection::ConnectionStatus st =
      connection_->BindAndBlockingConnect(remote, local, 1000);
  return st == connection::ConnectionStatus::kError ? CliStatus::kError
                                                    : CliStatus::kOk;
}

void RedisCli::AddCommand(const std::string& cmd) {
  query_buf_->WriteToBuffer(cmd.c_str(), cmd.size());
}

std::string RedisCli::GetReply() {
  // Drain any buffered complete reply before touching the socket.
  const auto result = MaybeGetReply();
  return result.has_value() ? *result : GetReplyFromConnection();
}

CompletableFuture<std::string> RedisCli::GetReplyAsync() {
  auto future =
      std::async(std::launch::async, [&]() { return GetReplyAsyncCallback(); });
  return CompletableFuture<std::string>(std::move(future));
}

std::string RedisCli::GetReplyAsyncCallback() {
  const std::lock_guard<std::mutex> lock(lock_);
  return GetReply();
}

std::optional<std::string> RedisCli::MaybeGetReply() {
  std::vector<std::string> reply;
  if (reply_buf_ && !reply_buf_->Empty() && ProcessReply(reply)) {
    return ReplyListToString(reply);
  }
  return std::nullopt;
}

std::string RedisCli::GetReplyFromConnection() {
  std::vector<std::string> reply;
  if (!query_buf_->Empty()) {
    ssize_t sent =
        WriteToConnection(connection_.get(), query_buf_->GetBufInString());
    if (sent <= 0) {
      return ErrResp;
    }
    if (sent == query_buf_->NRead()) {
      query_buf_->Clear();
    } else {
      query_buf_->IncrProcessedOffset(sent);
      query_buf_->TrimProcessedBuffer();
    }
    const auto replies = ReadFromConnection(connection_.get());
    reply_buf_->WriteToBuffer(replies.c_str(), replies.size());
    if (ProcessReply(reply)) {
      return ReplyListToString(reply);
    }
  }
  return NoReplyResp;
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
