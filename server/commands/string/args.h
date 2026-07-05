#pragma once

#include <cstdint>
#include <string>

namespace redis_simple::command::strings {
struct StringArgs {
  std::string key;
  std::string value;
  int64_t expire{0};
  int flags{0};
};
}  // namespace redis_simple::command::strings
