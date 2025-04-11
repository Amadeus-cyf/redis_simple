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
    std::memset(buf, 0, sizeof(buf));
    if (reply.size() > 0 && reply[reply.size() - 2] == '\r' &&
        reply[reply.size() - 1] == '\n') {
      break;
    }
  }
  return reply;
}

ssize_t WriteToConnection(const connection::Connection* connection,
                          const std::string& cmds) {
  ssize_t nwritten = 0;
  printf("client write %s\n", cmds.c_str());
  while (nwritten < cmds.size()) {
    ssize_t n = connection->SyncWrite(cmds.c_str(), cmds.size(), 1000);
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
    reply_str.append(std::move(r)).push_back('\n');
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
  const connection::Context& ctx = {
      .fd = -1, .event_loop = std::shared_ptr<ae::AeEventLoop>()};
  connection_ = std::make_unique<connection::Connection>(ctx);
  const connection::AddressInfo remote(ip, port);
  std::optional<const connection::AddressInfo> local = std::nullopt;
  if (ip_.has_value() && port_.has_value()) {
    local.emplace(connection::AddressInfo(ip_.value(), port_.value()));
  }
  connection::StatusCode st =
      connection_->BindAndBlockingConnect(remote, local, 1000);
  return st == connection::StatusCode::connStatusErr ? CliStatus::cliErr
                                                     : CliStatus::cliOK;
}

void RedisCli::AddCommand(const std::string& cmd) {
  query_buf_->WriteToBuffer(cmd.c_str(), cmd.size());
}

std::string RedisCli::GetReply() {
  // Try to get reply from local buffer first. If failed, get reply through
  // sending queries to the server
  const std::optional<const std::string>& opt = MaybeGetReply();
  return opt.value_or(GetReplyFromConnection());
}

CompletableFuture<std::string> RedisCli::GetReplyAsync() {
  std::future<std::string> future =
      std::async(std::launch::async, [&]() { return GetReplyAsyncCallback(); });
  return CompletableFuture<std::string>(std::move(future));
}

/*
 * Callback of GetReplyAsync(). Thread-safe version of GetReply().
 */
std::string RedisCli::GetReplyAsyncCallback() {
  const std::lock_guard<std::mutex> lock(lock_);
  return GetReply();
}

std::optional<const std::string> RedisCli::MaybeGetReply() {
  std::vector<std::string> reply;
  if (reply_buf_ && !reply_buf_->Empty() && ProcessReply(reply)) {
    const std::string& s = ReplyListToString(reply);
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
    // Clear sent queries. Clear the entire query buffer if all queries have
    // been sent.
    if (sent == query_buf_->NRead()) {
      query_buf_->Clear();
    } else {
      query_buf_->IncrProcessedOffset(sent);
      query_buf_->TrimProcessedBuffer();
    }
    const std::string& replies = ReadFromConnection(connection_.get());
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
