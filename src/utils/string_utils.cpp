#include "string_utils.h"

namespace redis_simple {
namespace utils {
std::vector<std::string> split(std::string s, std::string delimiter) {
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

void shiftCStr(char* s, size_t len, size_t offset) {
  if (offset <= 0) {
    return;
  }
  if (offset >= len) {
    memset(s, '\0', len);
    return;
  }
  memmove(s, s + offset, len - offset);
}

void touppercase(std::string& s) {
  std::transform(s.begin(), s.end(), s.begin(), toupper);
}
}  // namespace utils
}  // namespace redis_simple
