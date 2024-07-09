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
  /* spec for LIMIT flag */
  struct LimitSpec {
    size_t offset;
    ssize_t count;
    LimitSpec() : offset(0), count(0){};
    LimitSpec(size_t offset, size_t count) : offset(offset), count(count){};
  };
  /* spec for range by rank */
  struct RangeByRankSpec {
    long min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    /* starting offset and count */
    std::unique_ptr<LimitSpec> limit;
    /* reverse order ? */
    bool reverse;
  };
  /* spec for range by score */
  struct RangeByScoreSpec {
    double min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    /* starting offset and count */
    std::unique_ptr<LimitSpec> limit;
    /* reverse order ? */
    bool reverse;
  };
  /* entry storing key and score */
  struct ZSetEntry {
    ZSetEntry(const std::string& key, const double score)
        : key(key), score(score){};
    std::string key;
    mutable double score;
  };
  static ZSet* Init() { return new ZSet(); }
  void InsertOrUpdate(const std::string& key, const double score);
  bool Delete(const std::string& key);
  int GetRankOfKey(const std::string& key);
  const std::vector<const ZSetEntry*> RangeByRank(const RangeByRankSpec* spec);
  const std::vector<const ZSetEntry*> RangeByScore(
      const RangeByScoreSpec* spec);
  size_t Count(const RangeByScoreSpec* spec);
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
  ToSkiplistRangeByRankSpec(const RangeByRankSpec* spec);
  const in_memory::Skiplist<const ZSetEntry*, Comparator,
                            Destructor>::SkiplistRangeByKeySpec*
  ToSkiplistRangeByKeySpec(const RangeByScoreSpec* spec);
  void FreeSkiplistRangeByRankSpec(
      const in_memory::Skiplist<const ZSetEntry*, Comparator,
                                Destructor>::SkiplistRangeByRankSpec*
          skiplist_spec);
  void FreeSkiplistRangeByKeySpec(
      const in_memory::Skiplist<const ZSetEntry*, Comparator,
                                Destructor>::SkiplistRangeByKeySpec*
          skiplist_spec);
  /* dict mapping key to score */
  std::unique_ptr<in_memory::Dict<std::string, double>> dict_;
  /* skiplist storing key score pairs ordered by score */
  std::unique_ptr<in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>
      skiplist_;
  /* min and max key value, used for RangeByScore */
  mutable std::optional<std::string> max_key_, min_key_;
};
}  // namespace zset
}  // namespace redis_simple
