#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace redis_simple {
namespace utils {
std::vector<std::string> Split(const std::string& s,
                               const std::string& delimiter);
// Shift a buffer by offset. If offset >= buffer len, clear the entire buffer.
void ShiftCStr(char* s, size_t len, size_t offset);
// Turn a string to uppercase.
void ToUppercase(std::string& s);
// Return true if the string strictly represents a signed int64: no leading or
// trailing spaces, no extra characters, and no leading zeroes except "0".
bool ToInt64(const std::string& s, int64_t* const v);
// Convert long long to string and store it in the buffer dst
int LL2String(char* dst, size_t dstlen, long long svalue);
// Convert unsigned long long to string and store it in the buffer dst. Ref to
// the following article.
// https://engineering.fb.com/2013/03/15/developer-tools/three-optimization-tips-for-c/
int Ull2String(char* dst, size_t dstlen, unsigned long long value);
}  // namespace utils
}  // namespace redis_simple
