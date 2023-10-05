#pragma once

#include <string>

namespace redis_simple {
namespace reply {
enum ReplyStatus {
  replyOK = 1,
  replyErr = -1,
};

extern const std::string& CRLF;
std::string fromString(const std::string& s);
std::string fromBulkString(const std::string& s);
std::string fromInt64(const int64_t ll);
}  // namespace reply
}  // namespace redis_simple
