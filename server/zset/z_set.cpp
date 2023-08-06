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
  Comparator compare;
  const SkiplistEntry* inserted = skiplist->insert(se);
  if (inserted != se) {
    /* If the element already exists in the skiplist, update the score */
    inserted->score = se->score;
    delete se;
  }
}

bool ZSet::del(const std::string& key, const double score) const {
  if (dict->del(key) == in_memory::DictStatus::dictErr) {
    return false;
  }
  const SkiplistEntry se(key, score);
  return skiplist->del(&se);
}
}  // namespace z_set
}  // namespace redis_simple
