#pragma once

#include <sys/types.h>

#include <cstddef>
#include <memory>

namespace redis_simple {
namespace zset {
// Spec for LIMIT flag
struct LimitSpec {
  // 0-based index
  size_t offset;
  ssize_t count;
  LimitSpec() : offset(0), count(0) {}
  LimitSpec(size_t offset, ssize_t count) : offset(offset), count(count) {}
};

// Spec for range by rank
struct RangeByRankSpec {
  RangeByRankSpec()
      : min(0),
        max(0),
        minex(false),
        maxex(false),
        limit(nullptr),
        reverse(false) {}
  RangeByRankSpec(long min, long max, bool minex, bool maxex,
                  std::unique_ptr<LimitSpec> limit = nullptr,
                  bool reverse = false)
      : min(min),
        max(max),
        minex(minex),
        maxex(maxex),
        limit(std::move(limit)),
        reverse(reverse) {}
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
  RangeByScoreSpec()
      : min(0),
        max(0),
        minex(false),
        maxex(false),
        limit(nullptr),
        reverse(false) {}
  RangeByScoreSpec(double min, double max, bool minex, bool maxex,
                   std::unique_ptr<LimitSpec> limit = nullptr,
                   bool reverse = false)
      : min(min),
        max(max),
        minex(minex),
        maxex(maxex),
        limit(std::move(limit)),
        reverse(reverse) {}
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
