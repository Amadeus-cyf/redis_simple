#include "string_utils.h"

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
    memset(s, '\0', len);
    return;
  }
  memmove(s, s + offset, len - offset);
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
        printf("%lld, %lld\n", tmp, val);
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
}  // namespace utils
}  // namespace redis_simple
