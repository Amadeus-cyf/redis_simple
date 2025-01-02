#pragma once

#include <stdlib.h>

#include <string>
#include <vector>

namespace redis_simple {
namespace utils {
// Get number of digits in the number.
uint32_t Digits10(uint64_t v);
}  // namespace utils
}  // namespace redis_simple
