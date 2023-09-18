#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "memory/skiplist.h"

namespace redis_simple {
namespace zset {
class ZSet {
 public:
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
  std::vector<const ZSetEntry*> range(int start, int end) const {
    return skiplist->getElementsByRange(start, end);
  }
  std::vector<const ZSetEntry*> revrange(int start, int end) const {
    return skiplist->getElementsByRevRange(start, end);
  }
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
