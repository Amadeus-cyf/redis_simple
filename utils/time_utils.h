#pragma once

#include <chrono>

namespace redis_simple {
namespace utils {
inline int64_t GetNowInMilliseconds() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
}  // namespace utils
}  // namespace redis_simple
