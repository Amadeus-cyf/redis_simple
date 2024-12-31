#pragma once

#include <string>

namespace redis_simple {
namespace zset {
/* entry storing key and score */
struct ZSetEntry {
  ZSetEntry(const std::string& key, const double score)
      : key(key), score(score){};
  std::string key;
  mutable double score;
};
}  // namespace zset
}  // namespace redis_simple
