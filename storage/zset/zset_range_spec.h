#pragma once

#include <memory>

namespace redis_simple {
namespace zset {
// Spec for LIMIT flag
struct LimitSpec {
  // 0-based index
  size_t offset;
  ssize_t count;
  LimitSpec() : offset(0), count(0){};
  LimitSpec(size_t offset, size_t count) : offset(offset), count(count){};
};

// Spec for range by rank
struct RangeByRankSpec {
  // 0-based index
  long min, max;
  // Are min or max exclusive?
  bool minex, maxex;
  // Starting offset and count
  std::unique_ptr<LimitSpec> limit;
  // Reverse order?
  bool reverse;
};

// Spec for range by score
struct RangeByScoreSpec {
  double min, max;
  // Are min or max exclusive?
  bool minex, maxex;
  // Starting offset and count
  std::unique_ptr<LimitSpec> limit;
  // Reverse order?
  bool reverse;
};
}  // namespace zset
}  // namespace redis_simple
