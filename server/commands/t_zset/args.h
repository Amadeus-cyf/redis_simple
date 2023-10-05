#pragma once

#include <string>

namespace redis_simple {
namespace command {
namespace t_zset {
struct ZSetArgs {
  std::string key;
  std::string ele;
  double score;
};
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
