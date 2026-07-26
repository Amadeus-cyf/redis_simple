#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace redis_simple::reply {
enum class ProtocolVersion {
  kResp2 = 2,
  kResp3 = 3,
};

std::string FromString(std::string_view s);
std::string FromBulkString(std::string_view s);
std::string FromInt64(int64_t i64);
std::string FromArray(const std::vector<std::string>& array);
std::string FromBulkStringArray(const std::vector<std::string>& values);
std::string FromArrayHeader(size_t size);
void AppendArrayHeader(size_t size, std::string* reply);
void AppendBulkString(std::string_view s, std::string* reply);
std::string FromFloat(double fl, ProtocolVersion protocol);
std::string FromError(std::string_view message);
std::string WrongNumberOfArguments();
std::string UnknownCommand(std::string_view command);
std::string SyntaxError();
std::string WrongTypeError();
std::string Null(ProtocolVersion protocol);
std::string FromMapHeader(size_t size, ProtocolVersion protocol);
}  // namespace redis_simple::reply
