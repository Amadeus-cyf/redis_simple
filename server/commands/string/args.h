#pragma once
#include <string>

namespace redis_simple::command::strings {
struct StringArgs {
  std::string key;
  std::string value;
  int64_t expire;
};
}  // namespace redis_simple::command::strings
