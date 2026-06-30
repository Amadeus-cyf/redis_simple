#pragma once

#include "memory/listpack.h"
#include "storage/zset/zset_storage.h"

namespace redis_simple::zset {
class ZSetListPack : public ZSetStorage {
 public:
  ZSetListPack();
  bool InsertOrUpdate(const std::string& key, double score) override;
  bool Delete(const std::string& key) override;
  std::optional<double> Score(const std::string& key) const override;
  std::optional<size_t> Rank(const std::string& key) const override;
  ZSetEntryList RangeByRank(const RangeByRankSpec* spec) const override;
  ZSetEntryList RangeByScore(const RangeByScoreSpec* spec) const override;
  size_t Count(const RangeByScoreSpec* spec) const override;
  size_t Size() const override {
    // Since listpack include both keys and scores, the actual size should be
    // divided by 2.
    return listpack_->Size() / 2;
  };

 private:
  void DeleteKeyScorePair(size_t idx);
  double ScoreAt(size_t idx);
  ZSetEntryList RangeByRankUtil(const RangeByRankSpec* spec) const;
  ZSetEntryList RevRangeByRankUtil(const RangeByRankSpec* spec) const;
  ZSetEntryList RangeByScoreUtil(const RangeByScoreSpec* spec) const;
  ZSetEntryList RevRangeByScoreUtil(const RangeByScoreSpec* spec) const;
  const ZSetEntry* AddRangeResult(const std::string& key, double score) const;
  ssize_t FindKeyGreaterOrEqual(const RangeByScoreSpec* spec) const;
  ssize_t FindKeyLessOrEqual(const RangeByScoreSpec* spec) const;
  static bool ValidateRangeRankSpec(const RangeByRankSpec* spec);
  static bool ValidateRangeScoreSpec(const RangeByScoreSpec* spec);
  static bool IsInRange(const std::string& score, const RangeByScoreSpec* spec);
  static bool LessOrEqual(const std::string& score,
                          const RangeByScoreSpec* spec);
  // Listpack storing key score pairs
  std::unique_ptr<in_memory::ListPack> listpack_;
  mutable std::vector<std::unique_ptr<ZSetEntry>> range_cache_;
};
}  // namespace redis_simple::zset
