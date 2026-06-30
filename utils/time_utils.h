#pragma once

#include <chrono>

namespace redis_simple::utils {
inline int64_t NowInMilliseconds() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
}  // namespace redis_simple::utils
