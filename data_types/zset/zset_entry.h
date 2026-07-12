#pragma once

#include <string>
#include <string_view>

namespace redis_simple::zset {
// Entry storing key and score
struct ZSetEntry {
  ZSetEntry(const std::string& key, double score) : key(key), score(score) {}
  ZSetEntry(std::string_view key, double score)
      : key(key.data(), key.size()), score(score) {}
  std::string key;
  mutable double score;
};
}  // namespace redis_simple::zset
