#include <cstdint>
#include <string>
#include <vector>

namespace redis_simple::utils {
uint32_t Digits10(uint64_t v) {
  if (v < 10) {
    return 1;
  }
  if (v < 100) {
    return 2;
  }
  if (v < 1000) {
    return 3;
  }
  if (v < 1000000000000UL) {
    if (v < 100000000UL) {
      if (v < 1000000) {
        if (v < 10000) {
          return 4;
        }
        return 5 + static_cast<int>(v >= 100000);
      }
      return 7 + static_cast<int>(v >= 10000000UL);
    }
    if (v < 10000000000UL) {
      return 9 + static_cast<int>(v >= 1000000000UL);
    }
    return 11 + static_cast<int>(v >= 100000000000UL);
  }
  return 12 + Digits10(v / 1000000000000UL);
}
}  // namespace redis_simple::utils
