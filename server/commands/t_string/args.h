#pragma once
#include <string>

namespace redis_simple {
namespace command {
namespace t_string {
struct StrArgs {
  std::string key;
  std::string val;
  int64_t expire;
};
}  // namespace t_string
}  // namespace command
}  // namespace redis_simple
