#include "utils/string_utils.h"

#include <limits>

#include "utils/int_utils.h"

namespace redis_simple {
namespace utils {
std::vector<std::string> Split(std::string s, std::string delimiter) {
  if (delimiter.empty()) return {s};
  std::vector<std::string> res;
  auto start = 0, end = -1;
  while ((end = s.find(delimiter, start)) != std::string::npos) {
    res.push_back(s.substr(start, end - start));
    start = end + 1;
  }
  if (start < s.length()) {
    res.push_back(s.substr(start));
  }
  return res;
}

void ShiftCStr(char* s, size_t len, size_t offset) {
  if (!s || offset <= 0) {
    return;
  }
  if (offset >= len) {
    std::memset(s, '\0', len);
    return;
  }
  std::memmove(s, s + offset, len - offset);
}

void ToUppercase(std::string& s) {
  std::transform(s.begin(), s.end(), s.begin(), toupper);
}

bool ToInt64(const std::string& s, int64_t* const v) {
  /* directly return false if the string is empty or the number is overflow */
  if (s.empty() || s.size() > 20) return false;
  int sign = 1;
  int64_t val = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    if (i == 0 && (s[i] == '+' || s[i] == '-')) {
      sign = s[i] == '-' ? -1 : 1;
    } else if (i == 0 && s.size() > 1 && s[i] == '0') {
      return false;
    } else if (i == 1 && s.size() > 2 && (s[0] == '+' || s[0] == '-') &&
               s[i] == '0') {
      return false;
    } else if (s[i] >= '0' && s[i] <= '9') {
      int64_t tmp = val * 10 + sign * (s[i] - '0');
      /* check integer overflow */
      if ((tmp < 0 && val > 0) || (tmp > 0 && val < 0)) {
        return false;
      }
      val = tmp;
    } else {
      return false;
    }
  }
  if (v) *v = val;
  return true;
}

int LL2String(char* dst, size_t dstlen, long long svalue) {
  unsigned long long value;
  int negative = 0;
  if (svalue < 0) {
    if (svalue != std::numeric_limits<long long>::min()) {
      value = -svalue;
    } else {
      value = (unsigned long long)std::numeric_limits<long long>::max() + 1;
    }
    if (dstlen < 2) {
      if (dstlen > 0) dst[0] = '\0';
      return 0;
    }
    negative = 1;
    dst[0] = '-';
    ++dst;
    --dstlen;
  } else {
    value = svalue;
  }
  int length = Ull2String(dst, dstlen, value);
  if (length == 0) return 0;
  return length + negative;
}

int Ull2String(char* dst, size_t dstlen, unsigned long long value) {
  static const char digits[201] =
      "0001020304050607080910111213141516171819"
      "2021222324252627282930313233343536373839"
      "4041424344454647484950515253545556575859"
      "6061626364656667686970717273747576777879"
      "8081828384858687888990919293949596979899";
  /* Check length */
  uint32_t length = Digits10(value);
  if (length >= dstlen) {
    if (dstlen > 0) {
      dst[0] = '\0';
    }
    return 0;
  }
  /* Null term */
  uint32_t next = length - 1;
  dst[next + 1] = '\0';
  while (value >= 100) {
    int const i = (value % 100) * 2;
    value /= 100;
    dst[next] = digits[i + 1];
    dst[next - 1] = digits[i];
    next -= 2;
  }
  /* Handle last 1-2 digits */
  if (value < 10) {
    dst[next] = '0' + static_cast<uint32_t>(value);
  } else {
    int i = static_cast<uint32_t>(value) * 2;
    dst[next] = digits[i + 1];
    dst[next - 1] = digits[i];
  }
  return length;
}
}  // namespace utils
}  // namespace redis_simple
