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
/* Return true if the string could be converted into a signed int64.
 * Note that this function demands that the string strictly represents
 * a int64 value: no spaces or other characters before or after the string
 * representing the number are accepted, nor zeroes at the start if not
 * for the string "0" representing the zero number.
 */
bool ToInt64(const std::string& s, int64_t* const v);
/*  Convert long long to string and store it in the buffer dst */
int ll2string(char* dst, size_t dstlen, long long svalue);
/* Convert unsigned long long to string and store it in the buffer dst. Ref to
 * the following article.
 * https://engineering.fb.com/2013/03/15/developer-tools/three-optimization-tips-for-c/
 */
int ull2string(char* dst, size_t dstlen, unsigned long long value);
}  // namespace utils
}  // namespace redis_simple
