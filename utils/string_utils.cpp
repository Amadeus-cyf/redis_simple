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
}  // namespace utils
}  // namespace redis_simple
