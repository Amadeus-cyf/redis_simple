#pragma once

#include <stdlib.h>

#include <string>
#include <vector>

namespace redis_simple {
namespace utils {
/* Split a string by delimiter. */
std::vector<std::string> Split(std::string s, std::string delimiter);
/* Shift a buffer by offset. If offset >= buffer len, clear the entire buffer.
 */
void ShiftCStr(char* s, size_t len, size_t offset);
/* Turn a string to uppercase.*/
void ToUppercase(std::string& s);
}  // namespace utils
}  // namespace redis_simple
