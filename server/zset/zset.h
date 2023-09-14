#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "memory/skiplist.h"

namespace redis_simple {
namespace zset {
class ZSet {
 public:
  struct SkiplistEntry {
    SkiplistEntry(const std::string& key, const double score)
        : key(key), score(score){};
    std::string key;
    mutable double score;
  };
  static ZSet* init() { return new ZSet(); }
  void addOrUpdate(const std::string& key, const double score) const;
  bool remove(const std::string& key) const;
  int getRank(const std::string& key) const;
  std::vector<const SkiplistEntry*> range(int start, int end) const {
    return skiplist->getElementsByRange(start, end);
  }
  std::vector<const SkiplistEntry*> revrange(int start, int end) const {
    return skiplist->getElementsByRevRange(start, end);
  }
  size_t size() const { return skiplist->size(); }

 private:
  struct Comparator {
    int operator()(const SkiplistEntry* s1, const SkiplistEntry* s2) const {
      if (s1->score < s2->score) return -1;
      if (s1->score > s2->score) return 1;
      return s1->key.compare(s2->key);
    }
  };

  struct Destructor {
    void operator()(const SkiplistEntry* se) const { delete se; }
  };

  explicit ZSet();
  std::unique_ptr<in_memory::Dict<std::string, double>> dict;
  std::unique_ptr<
      in_memory::Skiplist<const SkiplistEntry*, Comparator, Destructor>>
      skiplist;
};
}  // namespace zset
}  // namespace redis_simple
