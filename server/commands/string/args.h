#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace redis_simple::command::strings {
struct StringArgs {
  std::string_view key;
  std::string_view value;
  int64_t expire{0};
  int flags{0};
};
}  // namespace redis_simple::command::strings
