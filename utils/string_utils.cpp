#include "utils/string_utils.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string_view>

#include "utils/int_utils.h"

namespace redis_simple::utils {
namespace {
size_t SplitTokenCount(std::string_view s, std::string_view delimiter) {
  if (delimiter.empty()) {
    return 1;
  }

  size_t count = 0;
  size_t start = 0;
  size_t end = std::string_view::npos;
  while ((end = s.find(delimiter, start)) != std::string_view::npos) {
    ++count;
    start = end + delimiter.size();
  }
  if (start < s.length()) {
    ++count;
  }
  return count;
}
}  // namespace

std::vector<std::string> Split(const std::string& s,
                               const std::string& delimiter) {
  if (delimiter.empty()) {
    return {s};
  }
  std::vector<std::string> res;
  res.reserve(SplitTokenCount(s, delimiter));
  size_t start = 0;
  size_t end = std::string::npos;
  while ((end = s.find(delimiter, start)) != std::string::npos) {
    res.emplace_back(s.substr(start, end - start));
    start = end + delimiter.size();
  }
  if (start < s.length()) {
    res.emplace_back(s.substr(start));
  }
  return res;
}

std::vector<std::string_view> SplitView(std::string_view s,
                                        std::string_view delimiter) {
  if (delimiter.empty()) {
    return {s};
  }
  std::vector<std::string_view> res;
  res.reserve(SplitTokenCount(s, delimiter));
  size_t start = 0;
  size_t end = std::string_view::npos;
  while ((end = s.find(delimiter, start)) != std::string_view::npos) {
    res.emplace_back(s.substr(start, end - start));
    start = end + delimiter.size();
  }
  if (start < s.length()) {
    res.emplace_back(s.substr(start));
  }
  return res;
}

void ShiftCString(char* s, size_t len, size_t offset) {
  if ((s == nullptr) || offset == 0) {
    return;
  }
  if (offset >= len) {
    std::memset(s, '\0', len);
    return;
  }
  std::memmove(s, s + offset, len - offset);
}

void ToUppercase(std::string& s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) -> char {
    return static_cast<char>(std::toupper(c));
  });
}

bool ToInt64(std::string_view s, int64_t* const v) {
  if (s.empty() || s.size() > 20) {
    return false;
  }
  const bool has_sign = s[0] == '+' || s[0] == '-';
  if (has_sign && s.size() == 1) {
    return false;
  }
  const bool negative = s[0] == '-';
  const uint64_t max_magnitude =
      negative ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1
               : static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
  uint64_t magnitude = 0;
  for (size_t i = has_sign ? 1 : 0; i < s.size(); ++i) {
    const bool has_redundant_leading_zero =
        (s[i] == '0') &&
        ((i == 0 && s.size() > 1) || (i == 1 && has_sign && s.size() > 2));
    if (has_redundant_leading_zero) {
      return false;
    }
    if (s[i] < '0' || s[i] > '9') {
      return false;
    }
    const auto digit = static_cast<uint64_t>(s[i] - '0');
    if (magnitude > (max_magnitude - digit) / 10) {
      return false;
    }
    magnitude = (magnitude * 10) + digit;
  }
  if (v != nullptr) {
    if (negative && magnitude == max_magnitude) {
      *v = std::numeric_limits<int64_t>::min();
    } else {
      const auto value = static_cast<int64_t>(magnitude);
      *v = negative ? -value : value;
    }
  }
  return true;
}

int Int64ToString(char* dst, size_t dstlen, int64_t svalue) {
  uint64_t value = 0;
  int negative = 0;
  if (svalue < 0) {
    if (svalue != std::numeric_limits<int64_t>::min()) {
      value = -svalue;
    } else {
      value = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1;
    }
    if (dstlen < 2) {
      if (dstlen > 0) {
        dst[0] = '\0';
      }
      return 0;
    }
    negative = 1;
    dst[0] = '-';
    ++dst;
    --dstlen;
  } else {
    value = svalue;
  }
  const int length = Uint64ToString(dst, dstlen, value);
  if (length == 0) {
    return 0;
  }
  return length + negative;
}

int Uint64ToString(char* dst, size_t dstlen, uint64_t value) {
  static constexpr std::string_view kDigits =
      "0001020304050607080910111213141516171819"
      "2021222324252627282930313233343536373839"
      "4041424344454647484950515253545556575859"
      "6061626364656667686970717273747576777879"
      "8081828384858687888990919293949596979899";
  // Check length.
  uint32_t length = Digits10(value);
  if (length >= dstlen) {
    if (dstlen > 0) {
      dst[0] = '\0';
    }
    return 0;
  }
  // Null term.
  uint32_t next = length - 1;
  dst[next + 1] = '\0';
  while (value >= 100) {
    const int i = static_cast<int>((value % 100) * 2);
    value /= 100;
    dst[next] = kDigits[i + 1];
    dst[next - 1] = kDigits[i];
    next -= 2;
  }
  // Handle the last 1-2 digits.
  if (value < 10) {
    dst[next] = static_cast<char>('0' + static_cast<uint32_t>(value));
  } else {
    const int i = static_cast<int>(static_cast<uint32_t>(value) * 2);
    dst[next] = kDigits[i + 1];
    dst[next - 1] = kDigits[i];
  }
  return static_cast<int>(length);
}
}  // namespace redis_simple::utils
