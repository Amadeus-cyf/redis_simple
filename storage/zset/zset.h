#pragma once

#include <memory>
#include <optional>
#include <string>

#include "storage/zset/zset_entry.h"
#include "storage/zset/zset_range_spec.h"
#include "storage/zset/zset_storage.h"

namespace redis_simple::zset {
class ZSet {
 public:
  enum class Encoding {
    kListPack,
    kSkiplist,
  };

  static std::unique_ptr<ZSet> Create() {
    return std::unique_ptr<ZSet>(new ZSet());
  }
  bool InsertOrUpdate(const std::string& key, double score);
  bool Delete(const std::string& key);
  std::optional<double> Score(const std::string& key) const;
  std::optional<size_t> Rank(const std::string& key) const;
  ZSetEntryList RangeByRank(const RangeByRankSpec* spec) const;
  ZSetEntryList RangeByScore(const RangeByScoreSpec* spec) const;
  size_t Count(const RangeByScoreSpec* spec) const;
  size_t Size() const { return storage_->Size(); }
  Encoding Encoding() const;

 private:
  static constexpr size_t kListPackMaxEntries = 128;
  static constexpr size_t kListPackMaxElementLength = 64;
  ZSet();
  bool ShouldConvertToSkiplist(const std::string& key, bool inserted) const;
  void ConvertAndExpand();
  enum Encoding encoding_;
  std::unique_ptr<ZSetStorage> storage_;
};
}  // namespace redis_simple::zset
