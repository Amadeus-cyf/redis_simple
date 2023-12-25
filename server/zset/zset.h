#pragma once

#include <memory>
#include <optional>
#include <string>

#include "memory/dict.h"
#include "memory/skiplist.h"

namespace redis_simple {
namespace zset {
class ZSet {
 public:
  struct LimitSpec {
    long offset, count;
    LimitSpec() : offset(0), count(0){};
    LimitSpec(long offset, long count) : offset(offset), count(count){};
  };
  struct RangeByRankSpec {
    long min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    std::unique_ptr<LimitSpec> limit;
    /* reverse order ? */
    bool reverse;
  };
  struct RangeByScoreSpec {
    double min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    std::unique_ptr<LimitSpec> limit;
    /* reverse order ? */
    bool reverse;
  };
  struct ZSetEntry {
    ZSetEntry(const std::string& key, const double score)
        : key(key), score(score){};
    std::string key;
    mutable double score;
  };
  static ZSet* Init() { return new ZSet(); }
  void AddOrUpdate(const std::string& key, const double score) const;
  bool Remove(const std::string& key) const;
  int GetRankOfKey(const std::string& key) const;
  const std::vector<const ZSetEntry*> RangeByRank(
      const RangeByRankSpec* spec) const;
  const std::vector<const ZSetEntry*> RangeByScore(
      const RangeByScoreSpec* spec) const;
  long Count(const RangeByScoreSpec* spec) const;
  size_t Size() const { return skiplist_->Size(); }

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
  explicit ZSet();
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
  std::unique_ptr<in_memory::Dict<std::string, double>> dict_;
  std::unique_ptr<in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>
      skiplist_;
  mutable std::optional<std::string> max_key_, min_key_;
};
}  // namespace zset
}  // namespace redis_simple
