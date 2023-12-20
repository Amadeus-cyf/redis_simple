#include "resp_parser.h"

namespace redis_simple {
namespace cli {
namespace resp_parser {
static const std::string& Error = "error";
namespace {
struct Prefix {
  static constexpr const char StringPrefix = '+';
  static constexpr const char BulkStringPrefix = '$';
  static constexpr const char Int64Prefix = ':';
};

ssize_t findCRLF(const std::string& resp, int start) {
  while (start >= 0) {
    start = resp.find('\n', start);
    if (start < 0) break;
    if (start > 0 && resp[start - 1] == '\r') return start;
    ++start;
  }
  return -1;
}

ssize_t parseString(const std::string& resp, std::string& reply) {
  int i = findCRLF(resp, 0);
  if (i < 0) return -1;
  reply = resp.substr(1, i - 2);
  return i + 1;
}

ssize_t parseBulkString(const std::string& resp, std::string& reply) {
  int i = findCRLF(resp, 0);
  if (i < 0) return -1;
  int len = 0;
  for (int j = 1; j < i - 1; ++j) {
    if (!std::isdigit(resp[j])) {
      return -1;
    }
    len = len * 10 + (resp[j] - '0');
  }
  int j = findCRLF(resp, i + len + 1);
  if (j < 0) return -1;
  reply = resp.substr(i + 1, len);
  return j + 1;
}

ssize_t parseInt64(const std::string& resp, std::string& reply) {
  int i = findCRLF(resp, 0);
  if (i < 0) return -1;
  int sign = 1, j = 1, r = 0;
  if (resp[1] == '-') {
    sign = -1, ++j;
  }
  while (j < i - 1) {
    if (!std::isdigit(resp[j])) {
      return -1;
    }
    r = r * 10 + (resp[j++] - '0');
  }
  reply = std::to_string(sign * r);
  return i + 1;
}
}  // namespace

ssize_t parse(const std::string& resp, std::string& reply) {
  if (resp.size() < 2 || resp[resp.size() - 2] != '\r' ||
      resp[resp.size() - 1] != '\n') {
    return -1;
  }
  switch (resp[0]) {
    case Prefix::StringPrefix:
      return parseString(resp, reply);
    case Prefix::BulkStringPrefix:
      return parseBulkString(resp, reply);
    case Prefix::Int64Prefix:
      return parseInt64(resp, reply);
    default:
      return -1;
  }
}
}  // namespace resp_parser
}  // namespace cli
}  // namespace redis_simple
