#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "memory/skiplist.h"

namespace redis_simple {
namespace zset {
class ZSet {
 public:
  struct RangeOption {
    int limit, offset, count;
  };
  struct RangeByIndexSpec {
    int min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    RangeOption* option;
  };
  struct RangeByKeySpec {
    std::string &min, &max;
    /* are min or max exclusive? */
    bool minex, maxex;
    RangeOption* option;
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
  std::vector<const ZSetEntry*> rangeByIndex(
      const RangeByIndexSpec* spec) const;
  std::vector<const ZSetEntry*> revrangeByIndex(
      const RangeByIndexSpec* spec) const;
  size_t size() const { return skiplist->size(); }

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
  std::unique_ptr<in_memory::Dict<std::string, double>> dict;
  std::unique_ptr<in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>
      skiplist;
};
}  // namespace zset
}  // namespace redis_simple
