#pragma once
#include <string>

namespace redis_simple::command::t_string {
struct StringArgs {
  std::string key;
  std::string val;
  int64_t expire;
};
}  // namespace redis_simple::command::t_string
