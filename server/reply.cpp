#include "server/reply.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "utils/float_utils.h"

namespace redis_simple::reply {
constexpr std::string_view kCrlf = "\r\n";
constexpr char kStringPrefix = '+';
constexpr char kBulkStringPrefix = '$';
constexpr char kInt64Prefix = ':';
constexpr char kArrayPrefix = '*';
constexpr char kMapPrefix = '%';
constexpr char kSetPrefix = '~';
constexpr char kDoublePrefix = ',';
constexpr char kNullPrefix = '_';
constexpr char kErrorPrefix = '-';

template <typename Integer>
void AppendInteger(Integer value, std::string* output) {
  std::array<char, std::numeric_limits<Integer>::digits10 + 3> buffer{};
  const auto result =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  if (result.ec != std::errc()) {
    throw std::runtime_error("failed to encode integer reply");
  }
  output->append(buffer.data(),
                 static_cast<size_t>(result.ptr - buffer.data()));
}

std::string FromString(std::string_view s) {
  std::string reply;
  reply.reserve(1 + s.size() + kCrlf.size());
  reply.push_back(kStringPrefix);
  reply.append(s.data(), s.size()).append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string FromBulkString(std::string_view s) {
  std::string reply;
  AppendBulkString(s, &reply);
  return reply;
}

std::string FromInt64(int64_t i64) {
  std::string reply;
  reply.push_back(kInt64Prefix);
  AppendInteger(i64, &reply);
  reply.append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string FromArray(const std::vector<std::string>& array) {
  std::string reply = FromArrayHeader(array.size());
  for (const std::string& str : array) {
    if (str.size() < kCrlf.size() ||
        str.compare(str.size() - kCrlf.size(), kCrlf.size(), kCrlf.data(),
                    kCrlf.size()) != 0) {
      throw std::invalid_argument("array element not encoded");
    }
    reply.append(str);
  }
  return reply;
}

std::string FromBulkStringArray(const std::vector<std::string>& values) {
  std::string reply = FromArrayHeader(values.size());
  for (const auto& value : values) {
    AppendBulkString(value, &reply);
  }
  return reply;
}

std::string FromArrayHeader(size_t size) {
  std::string reply;
  AppendArrayHeader(size, &reply);
  return reply;
}

void AppendArrayHeader(size_t size, std::string* const reply) {
  reply->push_back(kArrayPrefix);
  AppendInteger(size, reply);
  reply->append(kCrlf.data(), kCrlf.size());
}

void AppendBulkString(std::string_view s, std::string* const reply) {
  reply->push_back(kBulkStringPrefix);
  AppendInteger(s.size(), reply);
  reply->append(kCrlf.data(), kCrlf.size())
      .append(s.data(), s.size())
      .append(kCrlf.data(), kCrlf.size());
}

std::string FromFloat(double fl, ProtocolVersion protocol) {
  const std::string value = utils::FloatToString(fl);
  if (protocol == ProtocolVersion::kResp2) {
    return FromBulkString(value);
  }
  std::string reply;
  reply.push_back(kDoublePrefix);
  reply.append(value).append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string FromError(std::string_view message) {
  std::string reply;
  reply.push_back(kErrorPrefix);
  reply.append(message.data(), message.size())
      .append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string WrongNumberOfArguments() {
  return FromError("ERR wrong number of arguments");
}

std::string UnknownCommand(std::string_view command) {
  std::string message = "ERR unknown command '";
  message.append(command.data(), command.size()).push_back('\'');
  return FromError(message);
}

std::string SyntaxError() { return FromError("ERR syntax error"); }

std::string WrongTypeError() {
  return FromError(
      "WRONGTYPE Operation against a key holding the wrong kind of "
      "value");
}

std::string Null(ProtocolVersion protocol) {
  if (protocol == ProtocolVersion::kResp2) {
    return "$-1\r\n";
  }
  std::string reply(1, kNullPrefix);
  reply.append(kCrlf);
  return reply;
}

std::string FromMapHeader(size_t size, ProtocolVersion protocol) {
  if (protocol == ProtocolVersion::kResp2) {
    if (size > std::numeric_limits<size_t>::max() / 2) {
      throw std::length_error("map reply size overflow");
    }
    return FromArrayHeader(size * 2);
  }
  std::string reply;
  reply.push_back(kMapPrefix);
  AppendInteger(size, &reply);
  reply.append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string FromSetHeader(size_t size, ProtocolVersion protocol) {
  if (protocol == ProtocolVersion::kResp2) {
    return FromArrayHeader(size);
  }
  std::string reply;
  reply.push_back(kSetPrefix);
  AppendInteger(size, &reply);
  reply.append(kCrlf.data(), kCrlf.size());
  return reply;
}
}  // namespace redis_simple::reply
