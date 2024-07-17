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
  static constexpr const char arrayPrefix = '*';
};

ssize_t Parse(const std::string& resp, size_t start,
              std::vector<std::string>* const reply);
ssize_t FindCRLF(const std::string& resp, size_t start);
ssize_t ParseString(const std::string& resp, size_t start,
                    std::vector<std::string>* const reply);
ssize_t ParseBulkString(const std::string& resp, size_t start,
                        std::vector<std::string>* const reply);
ssize_t ParseInt64(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply);
ssize_t ParseArray(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply);

ssize_t Parse(const std::string& resp, size_t start,
              std::vector<std::string>* const reply) {
  if (start > resp.size() - 2 || resp[resp.size() - 2] != '\r' ||
      resp[resp.size() - 1] != '\n') {
    return -1;
  }
  switch (resp[start]) {
    case Prefix::stringPrefix:
      return ParseString(resp, start, reply);
    case Prefix::bulkStringPrefix:
      return ParseBulkString(resp, start, reply);
    case Prefix::int64Prefix:
      return ParseInt64(resp, start, reply);
    case Prefix::arrayPrefix:
      return ParseArray(resp, start, reply);
    default:
      return -1;
  }
}

ssize_t FindCRLF(const std::string& resp, size_t start) {
  start = resp.find_first_of('\r', start);
  if (start == std::string::npos) return -1;
  if (start < resp.size() - 1 && resp[start + 1] == '\n') return start;
  return -1;
}

ssize_t ParseString(const std::string& resp, size_t start,
                    std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) return -1;
  reply->push_back(resp.substr(start + 1, i - start - 1));
  return i - start + 2;
}

ssize_t ParseBulkString(const std::string& resp, size_t start,
                        std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) return -1;
  size_t len = 0;
  for (size_t j = start + 1; j < i; ++j) {
    if (!std::isdigit(resp[j])) return -1;
    len = len * 10 + (resp[j] - '0');
  }
  ssize_t j = i;
  while (j >= 0 && j < i + len + 2) {
    j = FindCRLF(resp, j + 2);
  }
  if (j < 0 || j != i + len + 2) return -1;
  reply->push_back(resp.substr(i + 2, len));
  return j - start + 2;
}

ssize_t ParseInt64(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) return -1;
  int sign = 1;
  long r = 0;
  for (size_t j = start + 1; j < i; ++j) {
    if (j == start + 1 && resp[j] == '-') {
      sign = -1;
      continue;
    }
    if (!std::isdigit(resp[j])) return -1;
    r = r * 10 + (resp[j] - '0');
  }
  reply->push_back(std::to_string(sign * r));
  return i - start + 2;
}

ssize_t ParseArray(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) return -1;
  std::string len_str;
  size_t len = 0, parsed = i - start + 2;
  for (size_t j = start + 1; j < i; ++j) {
    if (!std::isdigit(resp[j])) return -1;
    len = len * 10 + (resp[j++] - '0');
  }
  for (size_t j = 0; j < len; ++j) {
    ssize_t n = Parse(resp, start + parsed, reply);
    if (n < 0) return -1;
    parsed += n;
  }
  // indicate the end of the array
  reply->push_back("\n");
  return parsed;
}
}  // namespace

ssize_t Parse(const std::string& resp, std::vector<std::string>& reply) {
  return Parse(resp, 0, &reply);
}
}  // namespace resp_parser
}  // namespace cli
}  // namespace redis_simple
