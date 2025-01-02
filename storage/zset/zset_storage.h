#pragma once

#include <memory>
#include <optional>
#include <string>

#include "storage/zset/zset_entry.h"
#include "storage/zset/zset_range_spec.h"

namespace redis_simple {
namespace zset {
class ZSetStorage {
 public:
  // Insert or update a key with the given score. Return true if the key is
  // newly inserted.
  virtual bool InsertOrUpdate(const std::string& key, const double score) = 0;
  // Return true if the key is deleted.
  virtual bool Delete(const std::string& key) = 0;
  // Return the score of the given key.
  virtual std::optional<double> GetScoreOfKey(const std::string& key) const = 0;
  // Return the index of the given key.
  virtual std::optional<size_t> GetRankOfKey(const std::string& key) const = 0;
  // Return a list of keys within the given index range.
  virtual std::vector<const ZSetEntry*> RangeByRank(
      const RangeByRankSpec* spec) const = 0;
  // Get a list of keys within the given score range.
  virtual std::vector<const ZSetEntry*> RangeByScore(
      const RangeByScoreSpec* spec) const = 0;
  // Count the number of keys within the given range of score.
  virtual size_t Count(const RangeByScoreSpec* spec) const = 0;
  // Return the total number of keys.
  virtual size_t Size() const = 0;
  virtual ~ZSetStorage() = default;
};
}  // namespace zset
}  // namespace redis_simple
