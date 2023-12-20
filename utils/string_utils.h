#pragma once

#include <stdlib.h>

#include <string>
#include <vector>

namespace redis_simple {
namespace utils {
std::vector<std::string> Split(std::string s, std::string delimiter);
void ShiftCStr(char* s, size_t len, size_t offset);
void ToUppercase(std::string& s);
}  // namespace utils
}  // namespace redis_simple
