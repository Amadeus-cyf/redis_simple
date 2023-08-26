#include "z_set.h"

namespace redis_simple {
namespace z_set {
ZSet::ZSet()
    : dict(in_memory::Dict<std::string, double>::init()),
      skiplist(std::make_unique<in_memory::Skiplist<const SkiplistEntry*,
                                                    Comparator, Destructor>>(
          in_memory::Skiplist<const SkiplistEntry*, Comparator,
                              Destructor>::InitSkiplistLevel,
          Comparator(), Destructor())){};

void ZSet::addOrUpdate(const std::string& key, const double score) const {
  in_memory::Dict<std::string, double>::DictEntry* de = dict->addOrFind(key);
  de->val = score;
  const SkiplistEntry* se = new SkiplistEntry(key, score);
  const SkiplistEntry* inserted = skiplist->insert(se);
  if (inserted != se) {
    /* If the element already exists in the skiplist, update the score */
    inserted->score = se->score;
    delete se;
  }
}

bool ZSet::remove(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict->find(key);
  if (!de) {
    return false;
  }
  const double score = de->val;
  const SkiplistEntry se(key, score);
  dict->del(key);
  return skiplist->del(&se);
}

int ZSet::getRank(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict->find(key);
  if (!de) {
    return -1;
  }
  const double score = de->val;
  const SkiplistEntry se(key, score);
  return skiplist->getRankofElement(&se);
}
}  // namespace z_set
}  // namespace redis_simple
