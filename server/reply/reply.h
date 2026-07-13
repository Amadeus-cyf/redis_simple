#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace redis_simple::reply {
enum class ReplyStatus {
  kOk = 1,
  kError = -1,
};

constexpr int64_t ToInt(ReplyStatus status) {
  return static_cast<int64_t>(status);
}

std::string FromString(std::string_view s);
std::string FromBulkString(std::string_view s);
std::string FromInt64(int64_t i64);
std::string FromInt64(ReplyStatus status);
std::string FromArray(const std::vector<std::string>& array);
std::string FromBulkStringArray(const std::vector<std::string>& values);
std::string FromArrayHeader(size_t size);
void AppendBulkString(std::string_view s, std::string* reply);
std::string FromFloat(double fl);
std::string FromError(std::string_view message);
std::string WrongNumberOfArguments();
std::string UnknownCommand(std::string_view command);
std::string SyntaxError();
std::string WrongTypeError();
std::string Null();
}  // namespace redis_simple::reply
