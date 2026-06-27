#pragma once

#include <string>
#include <vector>

namespace redis_simple::reply {
enum class ReplyStatus {
  kOk = 1,
  kError = -1,
};

constexpr int64_t ToInt(ReplyStatus status) {
  return static_cast<int64_t>(status);
}

std::string FromString(const std::string& s);
std::string FromBulkString(const std::string& s);
std::string FromInt64(int64_t i64);
std::string FromInt64(ReplyStatus status);
std::string FromArray(const std::vector<std::string>& array);
std::string FromFloat(double fl);
std::string Null();
}  // namespace redis_simple::reply
