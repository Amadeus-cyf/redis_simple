#include "cli.h"

#include <arpa/inet.h>
#include <sys/types.h>

#include <array>
#include <cstdio>
#include <future>

#include "resp_parser.h"
#include "tcp/tcp.h"

namespace redis_simple::cli {
namespace {
std::string ReadFromConnection(const connection::Connection* connection) {
  std::string reply;
  ssize_t nread = 0;
  std::array<char, 4096> buf{};
  while ((nread = connection->SyncRead(buf.data(), buf.size(), 1000)) != EOF) {
    if (nread == 0) {
      break;
    }
    reply.append(buf.data(), nread);
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

RedisCli::RedisCli() : ip_(std::nullopt), port_(std::nullopt) {}

RedisCli::RedisCli(const std::string& ip, int port) : ip_(ip), port_(port) {}

CliStatus RedisCli::Connect(const std::string& ip, int port) {
  connection::Context ctx;
  ctx.event_loop = nullptr;
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
  const std::scoped_lock lock(lock_);
  query_buf_.Append(cmd.c_str(), cmd.size());
}

std::string RedisCli::ReadReply() {
  const std::scoped_lock lock(lock_);
  // Drain any buffered complete reply before touching the socket.
  const auto result = MaybeReadReply();
  return result.has_value() ? *result : ReadReplyFromConnection();
}

std::future<std::string> RedisCli::ReadReplyAsync() {
  return std::async(std::launch::async, [this]() { return ReadReply(); });
}

std::optional<std::string> RedisCli::MaybeReadReply() {
  std::vector<std::string> reply;
  if (!reply_buf_.Empty() && ProcessReply(reply)) {
    return ReplyListToString(reply);
  }
  return std::nullopt;
}

std::string RedisCli::ReadReplyFromConnection() {
  std::vector<std::string> reply;
  if (!query_buf_.Empty()) {
    ssize_t sent = WriteToConnection(connection_.get(), query_buf_.ToString());
    if (sent <= 0) {
      return kErrResp;
    }
    if (sent == query_buf_.Size()) {
      query_buf_.Clear();
    } else {
      query_buf_.Consume(sent);
      query_buf_.Compact();
    }
    const auto replies = ReadFromConnection(connection_.get());
    reply_buf_.Append(replies.c_str(), replies.size());
    if (ProcessReply(reply)) {
      return ReplyListToString(reply);
    }
  }
  return kNoReplyResp;
}

bool RedisCli::ProcessReply(std::vector<std::string>& reply) {
  ssize_t processed = resp_parser::Parse(reply_buf_.ToString(), reply);
  if (processed > 0) {
    reply_buf_.Consume(processed);
    reply_buf_.Compact();
    return true;
  }
  return false;
}
}  // namespace redis_simple::cli
