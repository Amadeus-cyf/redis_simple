#pragma once

#include <string>

namespace redis_simple::zset {
// Entry storing key and score
struct ZSetEntry {
  ZSetEntry(const std::string& key, double score) : key(key), score(score) {}
  std::string key;
  mutable double score;
};
}  // namespace redis_simple::zset
