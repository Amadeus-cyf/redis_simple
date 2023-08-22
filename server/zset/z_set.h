#pragma once

#include <memory>
#include <string>

#include "memory/dict.h"
#include "memory/skiplist.h"

namespace redis_simple {
namespace z_set {
class ZSet {
 public:
  static ZSet* init() { return new ZSet(); }
  void addOrUpdate(const std::string& key, const double score) const;
  bool remove(const std::string& key) const;

 private:
  struct SkiplistEntry {
    SkiplistEntry(const std::string& key, const double score)
        : key(key), score(score){};
    std::string key;
    mutable double score;
  };

  struct Comparator {
    int operator()(const SkiplistEntry* s1, const SkiplistEntry* s2) const {
      return s1->key < s2->key ? -1 : (s1->key == s2->key ? 0 : 1);
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
}  // namespace z_set
}  // namespace redis_simple
