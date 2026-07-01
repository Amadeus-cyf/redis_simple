#include "reply.h"

#include <sstream>
#include <string_view>

#include "utils/float_utils.h"

namespace redis_simple::reply {
constexpr std::string_view kCrlf = "\r\n";
constexpr char kStringPrefix = '+';
constexpr char kBulkStringPrefix = '$';
constexpr char kInt64Prefix = ':';
constexpr char kArrayPrefix = '*';
constexpr char kDoublePrefix = ',';
constexpr char kNullPrefix = '_';

std::string FromString(const std::string& s) {
  std::string reply;
  reply.push_back(kStringPrefix);
  reply.append(s).append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string FromBulkString(const std::string& s) {
  std::string reply;
  reply.push_back(kBulkStringPrefix);
  reply.append(std::to_string(s.size()))
      .append(kCrlf.data(), kCrlf.size())
      .append(s)
      .append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string FromInt64(int64_t i64) {
  std::string reply;
  reply.push_back(kInt64Prefix);
  reply.append(std::to_string(i64)).append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string FromInt64(ReplyStatus status) { return FromInt64(ToInt(status)); }

std::string FromArray(const std::vector<std::string>& array) {
  std::string reply;
  reply.push_back(kArrayPrefix);
  reply.append(std::to_string(array.size())).append(kCrlf.data(), kCrlf.size());
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

std::string FromFloat(double fl) {
  std::string reply;
  reply.push_back(kDoublePrefix);
  reply.append(utils::FloatToString(fl)).append(kCrlf.data(), kCrlf.size());
  return reply;
}

std::string Null() { return "_\r\n"; }
}  // namespace redis_simple::reply
