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

  static ZSet* Init() { return new ZSet(); }
  bool InsertOrUpdate(const std::string& key, double score);
  bool Delete(const std::string& key);
  std::optional<double> GetScoreOfKey(const std::string& key) const;
  std::optional<size_t> GetRankOfKey(const std::string& key) const;
  ZSetEntryList RangeByRank(const RangeByRankSpec* spec) const;
  ZSetEntryList RangeByScore(const RangeByScoreSpec* spec) const;
  size_t Count(const RangeByScoreSpec* spec) const;
  size_t Size() const { return storage_->Size(); }
  Encoding GetEncoding() const;

 private:
  enum class ZSetEncodingType {
    kListPack = 1,
    kSkiplist = 2,
  };
  static constexpr size_t kListPackMaxEntries = 128;
  static constexpr size_t kListPackMaxElementLength = 64;
  ZSet();
  bool ShouldConvertToSkiplist(const std::string& key, bool inserted) const;
  void ConvertAndExpand();
  ZSetEncodingType encoding_;
  std::unique_ptr<ZSetStorage> storage_;
};
}  // namespace redis_simple::zset
