#pragma once

#include <sys/types.h>

#include <optional>
#include <string_view>

#include "data_types/zset/zset_storage.h"
#include "memory/listpack.h"

namespace redis_simple::zset {
class ZSetListPack : public ZSetStorage {
 public:
  ZSetListPack();
  bool InsertOrUpdate(std::string_view key, double score) override;
  bool Delete(std::string_view key) override;
  std::optional<double> Score(std::string_view key) const override;
  std::optional<size_t> Rank(std::string_view key) const override;
  ZSetEntryList RangeByRank(const RangeByRankSpec* spec) const override;
  ZSetEntryList RangeByScore(const RangeByScoreSpec* spec) const override;
  size_t Count(const RangeByScoreSpec* spec) const override;
  size_t Size() const override {
    // Since listpack include both keys and scores, the actual size should be
    // divided by 2.
    return listpack_->Size() / 2;
  };

 private:
  struct EntryView {
    std::string_view key;
    double score;
    ssize_t key_index;
    ssize_t score_index;
  };

  void DeleteKeyScorePair(size_t idx);
  std::optional<std::string_view> ValueAt(size_t idx) const;
  std::optional<EntryView> EntryAt(ssize_t key_idx) const;
  std::optional<double> ScoreAt(size_t idx) const;
  ZSetEntryList RangeByRankUtil(const RangeByRankSpec* spec) const;
  ZSetEntryList RevRangeByRankUtil(const RangeByRankSpec* spec) const;
  ZSetEntryList RangeByScoreUtil(const RangeByScoreSpec* spec) const;
  ZSetEntryList RevRangeByScoreUtil(const RangeByScoreSpec* spec) const;
  const ZSetEntry* AddRangeResult(std::string_view key, double score) const;
  ssize_t NextKeyAfterScore(ssize_t score_idx) const;
  ssize_t PrevScoreBeforeKey(ssize_t key_idx) const;
  ssize_t PrevKeyBeforeKey(ssize_t key_idx) const;
  ssize_t FindKeyGreaterOrEqual(const RangeByScoreSpec* spec) const;
  ssize_t FindKeyLessOrEqual(const RangeByScoreSpec* spec) const;
  static bool ValidateRangeRankSpec(const RangeByRankSpec* spec);
  static bool ValidateRangeScoreSpec(const RangeByScoreSpec* spec);
  static bool IsInRange(double score, const RangeByScoreSpec* spec);
  static bool GreaterOrEqual(double score, const RangeByScoreSpec* spec);
  static bool LessOrEqual(double score, const RangeByScoreSpec* spec);
  // Listpack storing key score pairs
  std::unique_ptr<in_memory::ListPack> listpack_;
  mutable std::vector<std::unique_ptr<ZSetEntry>> range_cache_;
};
}  // namespace redis_simple::zset
