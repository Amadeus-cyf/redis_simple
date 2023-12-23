#include "resp_parser.h"

namespace redis_simple {
namespace cli {
namespace resp_parser {
static const std::string& Error = "error";
namespace {
struct Prefix {
  static constexpr const char stringPrefix = '+';
  static constexpr const char bulkStringPrefix = '$';
  static constexpr const char int64Prefix = ':';
};

static ssize_t FindCRLF(const std::string& resp, int start) {
  while (start >= 0) {
    start = resp.find('\n', start);
    if (start < 0) break;
    if (start > 0 && resp[start - 1] == '\r') return start;
    ++start;
  }
  return -1;
}

static ssize_t ParseString(const std::string& resp, std::string& reply) {
  int i = FindCRLF(resp, 0);
  if (i < 0) return -1;
  reply = resp.substr(1, i - 2);
  return i + 1;
}

static ssize_t ParseBulkString(const std::string& resp, std::string& reply) {
  int i = FindCRLF(resp, 0);
  if (i < 0) return -1;
  int len = 0;
  for (int j = 1; j < i - 1; ++j) {
    if (!std::isdigit(resp[j])) {
      return -1;
    }
    len = len * 10 + (resp[j] - '0');
  }
  int j = FindCRLF(resp, i + len + 1);
  if (j < 0) return -1;
  reply = resp.substr(i + 1, len);
  return j + 1;
}

static size_t ParseInt64(const std::string& resp, std::string& reply) {
  int i = FindCRLF(resp, 0);
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

ssize_t Parse(const std::string& resp, std::string& reply) {
  if (resp.size() < 2 || resp[resp.size() - 2] != '\r' ||
      resp[resp.size() - 1] != '\n') {
    return -1;
  }
  switch (resp[0]) {
    case Prefix::stringPrefix:
      return ParseString(resp, reply);
    case Prefix::bulkStringPrefix:
      return ParseBulkString(resp, reply);
    case Prefix::int64Prefix:
      return ParseInt64(resp, reply);
    default:
      return -1;
  }
}
}  // namespace resp_parser
}  // namespace cli
}  // namespace redis_simple
