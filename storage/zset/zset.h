#pragma once

#include <memory>
#include <optional>
#include <string>

#include "storage/zset/zset_entry.h"
#include "storage/zset/zset_range_spec.h"
#include "storage/zset/zset_storage.h"

namespace redis_simple {
namespace zset {
class ZSet {
 public:
  static ZSet* Init() { return new ZSet(); }
  bool InsertOrUpdate(const std::string& key, const double score);
  bool Delete(const std::string& key);
  std::optional<double> GetScoreOfKey(const std::string& key) const;
  std::optional<size_t> GetRankOfKey(const std::string& key) const;
  const std::vector<const ZSetEntry*> RangeByRank(
      const RangeByRankSpec* spec) const;
  const std::vector<const ZSetEntry*> RangeByScore(
      const RangeByScoreSpec* spec) const;
  size_t Count(const RangeByScoreSpec* spec) const;
  size_t Size() const { return storage_->Size(); };

 private:
  enum class ZSetEncodingType {
    ListPack = 1,
    Skiplist = 2,
  };
  static constexpr size_t ListPackMaxEntries = 128;
  ZSet();
  void ConvertAndExpand();
  /* zset encoding, could either be listpack or skiplist */
  ZSetEncodingType encoding_;
  /* data structure storing the data */
  std::unique_ptr<ZSetStorage> storage_;
};
}  // namespace zset
}  // namespace redis_simple
