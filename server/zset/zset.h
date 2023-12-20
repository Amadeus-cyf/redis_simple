#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "memory/skiplist.h"

namespace redis_simple {
namespace zset {
class ZSet {
 public:
  struct LimitSpec {
    long offset, count;
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
  static ZSet* init() { return new ZSet(); }
  void addOrUpdate(const std::string& key, const double score) const;
  bool remove(const std::string& key) const;
  int getRank(const std::string& key) const;
  std::vector<const ZSetEntry*> rangeByRank(const RangeByRankSpec* spec) const;
  std::vector<const ZSetEntry*> rangeByScore(
      const RangeByScoreSpec* spec) const;
  size_t size() const { return skiplist->Size(); }

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
  toSkiplistRangeByRankSpec(const RangeByRankSpec* spec) const;
  const in_memory::Skiplist<const ZSetEntry*, Comparator,
                            Destructor>::SkiplistRangeByKeySpec*
  toSkiplistRangeByKeySpec(const RangeByScoreSpec* spec) const;
  void freeSkiplistRangeByRankSpec(
      const in_memory::Skiplist<const ZSet::ZSetEntry*, ZSet::Comparator,
                                ZSet::Destructor>::SkiplistRangeByRankSpec*
          skiplist_spec) const;
  void freeSkiplistRangeByKeySpec(
      const in_memory::Skiplist<const ZSet::ZSetEntry*, ZSet::Comparator,
                                ZSet::Destructor>::SkiplistRangeByKeySpec*
          skiplist_spec) const;
  std::unique_ptr<in_memory::Dict<std::string, double>> dict;
  std::unique_ptr<in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>
      skiplist;
  mutable std::string max_key, min_key;
};
}  // namespace zset
}  // namespace redis_simple
