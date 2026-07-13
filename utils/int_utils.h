#pragma once

#include <cstdint>

namespace redis_simple::utils {
// Get number of digits in the number.
uint32_t Digits10(uint64_t v);
}  // namespace redis_simple::utils
