#include "resp_parser.h"

#include <cctype>
#include <limits>

namespace redis_simple::cli::resp_parser {
namespace {
struct Prefix {
  static constexpr char kStringPrefix = '+';
  static constexpr char kBulkStringPrefix = '$';
  static constexpr char kInt64Prefix = ':';
  static constexpr char kArrayPrefix = '*';
  static constexpr char kDoublePrefix = ',';
  static constexpr char kNullPrefix = '_';
};

constexpr char kNilResp[] = "(nil)";

ssize_t Parse(const std::string& resp, size_t start,
              std::vector<std::string>* reply);
ssize_t FindCRLF(const std::string& resp, size_t start);
ssize_t ParseString(const std::string& resp, size_t start,
                    std::vector<std::string>* reply);
ssize_t ParseBulkString(const std::string& resp, size_t start,
                        std::vector<std::string>* reply);
ssize_t ParseInt64(const std::string& resp, size_t start,
                   std::vector<std::string>* reply);
ssize_t ParseArray(const std::string& resp, size_t start,
                   std::vector<std::string>* reply);
ssize_t ParseNull(const std::string& resp, size_t start,
                  std::vector<std::string>* reply);
ssize_t ParseFloat(const std::string& resp, size_t start,
                   std::vector<std::string>* reply);

bool IsSign(char c) { return c == '+' || c == '-'; }

ssize_t ToSSize(size_t value) {
  if (value > static_cast<size_t>(std::numeric_limits<ssize_t>::max())) {
    return -1;
  }
  return static_cast<ssize_t>(value);
}

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
  if (start == std::string::npos) {
    return -1;
  }
  if (start < resp.size() - 1 && resp[start + 1] == '\n') {
    return ToSSize(start);
  }
  return -1;
}

ssize_t ParseString(const std::string& resp, size_t start,
                    std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) {
    return -1;
  }
  const auto end = static_cast<size_t>(i);
  reply->push_back(resp.substr(start + 1, end - start - 1));
  return ToSSize(end - start + 2);
}

ssize_t ParseBulkString(const std::string& resp, size_t start,
                        std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) {
    return -1;
  }
  const auto end = static_cast<size_t>(i);
  size_t len = 0;
  bool has_digit = false;
  for (size_t j = start + 1; j < end; ++j) {
    if (std::isdigit(static_cast<unsigned char>(resp[j])) == 0) {
      return -1;
    }
    has_digit = true;
    len = (len * 10) + (resp[j] - '0');
  }
  if (!has_digit) {
    return -1;
  }
  if (len > resp.size() || end + 2 > resp.size() - len) {
    return -1;
  }
  const size_t expected_end = end + len + 2;
  size_t next_end = end;
  while (next_end < expected_end) {
    const ssize_t found = FindCRLF(resp, next_end + 2);
    if (found < 0) {
      return -1;
    }
    next_end = static_cast<size_t>(found);
  }
  if (next_end != expected_end) {
    return -1;
  }
  reply->push_back(resp.substr(end + 2, len));
  return ToSSize(next_end - start + 2);
}

ssize_t ParseInt64(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) {
    return -1;
  }
  const auto end = static_cast<size_t>(i);
  int sign = 1;
  long num = 0;
  bool has_digit = false;
  for (size_t j = start + 1; j < end; ++j) {
    if (j == start + 1 && IsSign(resp[j])) {
      sign = resp[j] == '+' ? 1 : -1;
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(resp[j])) == 0) {
      return -1;
    }
    has_digit = true;
    num = (num * 10) + (resp[j] - '0');
  }
  if (!has_digit) {
    return -1;
  }
  reply->push_back(std::to_string(sign * num));
  return ToSSize(end - start + 2);
}

ssize_t ParseArray(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) {
    return -1;
  }
  const auto end = static_cast<size_t>(i);
  std::string len_str;
  size_t len = 0;
  size_t parsed = end - start + 2;
  bool has_digit = false;
  for (size_t j = start + 1; j < end; ++j) {
    if (std::isdigit(static_cast<unsigned char>(resp[j])) == 0) {
      return -1;
    }
    has_digit = true;
    len = (len * 10) + (resp[j] - '0');
  }
  if (!has_digit) {
    return -1;
  }
  for (size_t j = 0; j < len; ++j) {
    ssize_t n = Parse(resp, start + parsed, reply);
    if (n < 0) {
      return -1;
    }
    parsed += static_cast<size_t>(n);
  }
  // The CLI renderer uses this sentinel to add a line break after arrays.
  reply->emplace_back("\n");
  return ToSSize(parsed);
}

ssize_t ParseNull(const std::string& resp, size_t start,
                  std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0 || static_cast<size_t>(i) - start != 1) {
    return -1;
  }
  reply->emplace_back(kNilResp);
  return 3;
}

ssize_t ParseFloat(const std::string& resp, size_t start,
                   std::vector<std::string>* const reply) {
  ssize_t i = FindCRLF(resp, start);
  if (i < 0) {
    return -1;
  }
  const auto end = static_cast<size_t>(i);
  int sign = 1;
  int exponential_sign = 1;
  long integral = 0;
  long exponent = 0;
  std::string fractional;
  bool floating_point = false;
  bool exponential = false;
  bool has_digit = false;
  bool has_exponent_digit = false;
  for (size_t j = start + 1; j < end; ++j) {
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
      if (!has_digit || j == end - 1) {
        return -1;
      }
      if (IsSign(resp[j + 1])) {
        if (j + 1 == end - 1) {
          return -1;
        }
        exponential_sign = resp[j + 1] == '+' ? 1 : -1;
        ++j;
      } else if (std::isdigit(static_cast<unsigned char>(resp[j + 1])) == 0) {
        return -1;
      }
      exponential = true;
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(resp[j])) == 0) {
      return -1;
    }
    has_digit = true;
    if (floating_point && !exponential) {
      fractional.push_back(resp[j]);
    } else if (!floating_point && !exponential) {
      integral = (integral * 10) + (resp[j] - '0');
    } else {
      exponent = (exponent * 10) + (resp[j] - '0');
      has_exponent_digit = true;
    }
  }
  if (!has_digit || (exponential && !has_exponent_digit)) {
    return -1;
  }
  std::string floating_num_str = std::to_string(sign * integral);
  if (floating_point) {
    while (!fractional.empty() && fractional.back() == '0') {
      fractional.pop_back();
    }
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
  return ToSSize(end - start + 2);
}
}  // namespace

ssize_t Parse(const std::string& resp, std::vector<std::string>& reply) {
  return Parse(resp, 0, &reply);
}
}  // namespace redis_simple::cli::resp_parser
