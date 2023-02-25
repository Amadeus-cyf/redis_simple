#pragma once

#include <stdlib.h>

#include <string>
#include <vector>

namespace redis_simple {
namespace utils {
std::vector<std::string> split(std::string s, std::string delimiter);
void shiftCStr(char* s, size_t len, size_t offset);
void touppercase(std::string& s);
}  // namespace utils
}  // namespace redis_simple
