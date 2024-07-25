#pragma once

#include <string>
#include <vector>

namespace redis_simple {
namespace reply {
enum ReplyStatus {
  replyOK = 1,
  replyErr = -1,
};

std::string FromString(const std::string& s);
std::string FromBulkString(const std::string& s);
std::string FromInt64(const int64_t ll);
std::string FromArray(const std::vector<std::string>& array);
std::string FromFloat(const double fl);
std::string Null();
}  // namespace reply
}  // namespace redis_simple
