#pragma once

#include "memory/listpack.h"
#include "server/zset/zset_storage.h"

namespace redis_simple {
namespace zset {
class ZSetListPack : public ZSetStorage {
 public:
  ZSetListPack();
  bool InsertOrUpdate(const std::string& key, const double score) override;
  bool Delete(const std::string& key) override;
  std::optional<double> GetScoreOfKey(const std::string& key) const override;
  std::optional<size_t> GetRankOfKey(const std::string& key) const override;
  std::vector<const ZSetEntry*> RangeByRank(
      const RangeByRankSpec* spec) const override;
  std::vector<const ZSetEntry*> RangeByScore(
      const RangeByScoreSpec* spec) const override;
  size_t Count(const RangeByScoreSpec* spec) const override;
  size_t Size() const override {
    /* Since listpack include both keys and scores, the actual size should be
     * divided by 2. */
    return listpack_->GetNumOfElements() / 2;
  };

 private:
  void DeleteKeyScorePair(size_t idx);
  double GetScore(size_t idx);
  std::vector<const ZSetEntry*> RangeByRankUtil(
      const RangeByRankSpec* spec) const;
  std::vector<const ZSetEntry*> RevRangeByRankUtil(
      const RangeByRankSpec* spec) const;
  std::vector<const ZSetEntry*> RangeByScoreUtil(
      const RangeByScoreSpec* spec) const;
  std::vector<const ZSetEntry*> RevRangeByScoreUtil(
      const RangeByScoreSpec* spec) const;
  ssize_t FindKeyGreaterOrEqual(const RangeByScoreSpec* spec) const;
  ssize_t FindKeyLessOrEqual(const RangeByScoreSpec* spec) const;
  static bool ValidateRangeRankSpec(const RangeByRankSpec* spec);
  static bool ValidateRangeScoreSpec(const RangeByScoreSpec* spec);
  static bool IsInRange(const std::string& score, const RangeByScoreSpec* spec);
  static bool LessOrEqual(const std::string& score,
                          const RangeByScoreSpec* spec);
  /* listpack storing key score pairs */
  std::unique_ptr<in_memory::ListPack> listpack_;
};
}  // namespace zset
}  // namespace redis_simple
