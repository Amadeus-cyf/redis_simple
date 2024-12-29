#pragma once

#include <memory>

namespace redis_simple {
namespace zset {
/* spec for LIMIT flag */
struct LimitSpec {
  /* 0-based index */
  size_t offset;
  ssize_t count;
  LimitSpec() : offset(0), count(0){};
  LimitSpec(size_t offset, size_t count) : offset(offset), count(count){};
};

/* spec for range by rank */
struct RangeByRankSpec {
  /* 0-based index */
  long min, max;
  /* are min or max exclusive? */
  bool minex, maxex;
  /* starting offset and count */
  std::unique_ptr<LimitSpec> limit;
  /* reverse order ? */
  bool reverse;
};

/* spec for range by score */
struct RangeByScoreSpec {
  double min, max;
  /* are min or max exclusive? */
  bool minex, maxex;
  /* starting offset and count */
  std::unique_ptr<LimitSpec> limit;
  /* reverse order ? */
  bool reverse;
};
}  // namespace zset
}  // namespace redis_simple
