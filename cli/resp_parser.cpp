#include "resp_parser.h"

#include <cctype>

namespace redis_simple {
namespace cli {
namespace resp_parser {
namespace {
struct Prefix {
  static constexpr const char kStringPrefix = '+';
  static constexpr const char kBulkStringPrefix = '$';
  static constexpr const char kInt64Prefix = ':';
  static constexpr const char kArrayPrefix = '*';
  static constexpr const char kDoublePrefix = ',';
  static constexpr const char kNullPrefix = '_';
};

const std::string& kNilResp("(nil)");

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
ssize_t ParseNull(const std::string& resp, size_t start,
                  std::vector<std::string>* const reply);
ssize_t ParseFloat(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply);

bool IsSign(const char c) { return c == '+' || c == '-'; }

ssize_t Parse(const std::string& resp, size_t start,
              std::vector<std::string>* const reply) {
  if (resp.size() < 2 || start > resp.size() - 2 ||
      resp[resp.size() - 2] != '\r' || resp[resp.size() - 1] != '\n') {
    return -1;
  }
  switch (resp[start]) {
    case Prefix::kStringPrefix:
      return ParseString(resp, start, reply);
    case Prefix::kBulkStringPrefix:
      return ParseBulkString(resp, start, reply);
    case Prefix::kInt64Prefix:
      return ParseInt64(resp, start, reply);
    case Prefix::kArrayPrefix:
      return ParseArray(resp, start, reply);
    case Prefix::kNullPrefix:
      return ParseNull(resp, start, reply);
    case Prefix::kDoublePrefix:
      return ParseFloat(resp, start, reply);
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
  bool has_digit = false;
  for (size_t j = start + 1; j < i; ++j) {
    if (!std::isdigit(static_cast<unsigned char>(resp[j]))) return -1;
    has_digit = true;
    len = len * 10 + (resp[j] - '0');
  }
  if (!has_digit) return -1;
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
  long num = 0;
  bool has_digit = false;
  for (size_t j = start + 1; j < i; ++j) {
    if (j == start + 1 && IsSign(resp[j])) {
      sign = resp[j] == '+' ? 1 : -1;
      continue;
    }
    if (!std::isdigit(static_cast<unsigned char>(resp[j]))) return -1;
    has_digit = true;
    num = num * 10 + (resp[j] - '0');
  }
  if (!has_digit) return -1;
  reply->push_back(std::to_string(sign * num));
  return i - start + 2;
}

ssize_t ParseArray(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) return -1;
  std::string len_str;
  size_t len = 0, parsed = i - start + 2;
  bool has_digit = false;
  for (size_t j = start + 1; j < i; ++j) {
    if (!std::isdigit(static_cast<unsigned char>(resp[j]))) return -1;
    has_digit = true;
    len = len * 10 + (resp[j] - '0');
  }
  if (!has_digit) return -1;
  for (size_t j = 0; j < len; ++j) {
    ssize_t n = Parse(resp, start + parsed, reply);
    if (n < 0) return -1;
    parsed += n;
  }
  // The CLI renderer uses this sentinel to add a line break after arrays.
  reply->push_back("\n");
  return parsed;
}

ssize_t ParseNull(const std::string& resp, size_t start,
                  std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0 || i - start != 1) return -1;
  reply->push_back(kNilResp);
  return 3;
}

ssize_t ParseFloat(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) return -1;
  int sign = 1, exponential_sign = 1;
  long integral = 0, exponent = 0;
  std::string fractional;
  bool floating_point = false, exponential = false;
  bool has_digit = false, has_exponent_digit = false;
  for (size_t j = start + 1; j < i; ++j) {
    if (j == start + 1 && IsSign(resp[j])) {
      sign = resp[j] == '+' ? 1 : -1;
      continue;
    }
    if (!floating_point && !exponential && resp[j] == '.') {
      floating_point = true;
      continue;
    }
    if (!exponential &&
        std::tolower(static_cast<unsigned char>(resp[j])) == 'e') {
      if (!has_digit || j == i - 1) {
        return -1;
      }
      if (IsSign(resp[j + 1])) {
        if (j + 1 == i - 1) {
          return -1;
        }
        exponential_sign = resp[j + 1] == '+' ? 1 : -1;
        ++j;
      } else if (!std::isdigit(static_cast<unsigned char>(resp[j + 1]))) {
        return -1;
      }
      exponential = true;
      continue;
    }
    if (!std::isdigit(static_cast<unsigned char>(resp[j]))) return -1;
    has_digit = true;
    if (floating_point && !exponential) {
      fractional.push_back(resp[j]);
    } else if (!floating_point && !exponential) {
      integral = integral * 10 + (resp[j] - '0');
    } else {
      exponent = exponent * 10 + (resp[j] - '0');
      has_exponent_digit = true;
    }
  }
  if (!has_digit || (exponential && !has_exponent_digit)) {
    return -1;
  }
  std::string floating_num_str = std::to_string(sign * integral);
  if (floating_point) {
    while (!fractional.empty() && fractional.back() == '0')
      fractional.pop_back();
    if (!fractional.empty()) {
      floating_num_str.push_back('.');
      floating_num_str.append(fractional);
    }
  }
  if (exponential && exponent > 0) {
    floating_num_str.push_back('e');
    floating_num_str.append(std::to_string(exponential_sign * exponent));
  }
  reply->push_back(std::move(floating_num_str));
  return i - start + 2;
}
}  // namespace

ssize_t Parse(const std::string& resp, std::vector<std::string>& reply) {
  return Parse(resp, 0, &reply);
}
}  // namespace resp_parser
}  // namespace cli
}  // namespace redis_simple
