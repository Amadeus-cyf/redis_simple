#pragma once

#include "memory/dict.h"
#include "memory/skiplist.h"
#include "storage/zset/zset_storage.h"

namespace redis_simple {
namespace zset {
class ZSetSkiplist : public ZSetStorage {
 public:
  ZSetSkiplist();
  bool InsertOrUpdate(const std::string& key, const double score) override;
  bool Delete(const std::string& key) override;
  std::optional<double> GetScoreOfKey(const std::string& key) const override {
    return dict_->Get(key);
  }
  std::optional<size_t> GetRankOfKey(const std::string& key) const override;
  std::vector<const ZSetEntry*> RangeByRank(
      const RangeByRankSpec* spec) const override;
  std::vector<const ZSetEntry*> RangeByScore(
      const RangeByScoreSpec* spec) const override;
  size_t Count(const RangeByScoreSpec* spec) const override;
  size_t Size() const override { return skiplist_->Size(); }

 private:
  struct Comparator {
    int operator()(const ZSetEntry* s1, const ZSetEntry* s2) const {
      if (s1->score < s2->score) return -1;
      if (s1->score > s2->score) return 1;
      return s1->key.compare(s2->key);
    }
  };

  struct Destructor {
    void operator()(const ZSetEntry* se) const {
      printf("delete %s %f\n", se->key.c_str(), se->score);
      delete se;
      se = nullptr;
    }
  };

  const in_memory::Skiplist<const ZSetEntry*, Comparator,
                            Destructor>::SkiplistRangeByRankSpec*
  ToSkiplistRangeByRankSpec(const RangeByRankSpec* spec) const;
  const in_memory::Skiplist<const ZSetEntry*, Comparator,
                            Destructor>::SkiplistRangeByKeySpec*
  ToSkiplistRangeByKeySpec(const RangeByScoreSpec* spec) const;
  void FreeSkiplistRangeByRankSpec(
      const in_memory::Skiplist<const ZSetEntry*, Comparator,
                                Destructor>::SkiplistRangeByRankSpec*
          skiplist_spec) const;
  void FreeSkiplistRangeByKeySpec(
      const in_memory::Skiplist<const ZSetEntry*, Comparator,
                                Destructor>::SkiplistRangeByKeySpec*
          skiplist_spec) const;
  // Dict mapping key to score, used with the skiplist
  std::unique_ptr<in_memory::Dict<std::string, double>> dict_;
  // Skiplist storing key score pairs ordered by score
  std::unique_ptr<in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>
      skiplist_;
  // Min and max key value, used for RangeByScore
  mutable std::optional<std::string> max_key_, min_key_;
};
}  // namespace zset
}  // namespace redis_simple
