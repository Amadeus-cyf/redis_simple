#include "zset.h"

namespace redis_simple {
namespace zset {
ZSet::ZSet()
    : dict(in_memory::Dict<std::string, double>::init()),
      skiplist(std::make_unique<
               in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>(
          in_memory::Skiplist<const ZSetEntry*, Comparator,
                              Destructor>::InitSkiplistLevel,
          Comparator(), Destructor())){};

void ZSet::addOrUpdate(const std::string& key, const double score) const {
  in_memory::Dict<std::string, double>::DictEntry* de = dict->find(key);
  const ZSetEntry* se = new ZSetEntry(key, score);
  if (de) {
    printf("update %s's val from %f to %f\n", key.c_str(), de->val, score);
    if (de->val == score) return;
    const ZSetEntry old(key, de->val);
    bool r = skiplist->update(&old, se);
    assert(r);
  } else {
    printf("insert %s %f\n", key.c_str(), score);
    de = dict->addOrFind(key);
    const ZSetEntry* inserted = skiplist->insert(se);
    assert(inserted);
  }
  de->val = score;
}

bool ZSet::remove(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict->find(key);
  if (!de) {
    return false;
  }
  const double score = de->val;
  const ZSetEntry se(key, score);
  assert(dict->del(key));
  return skiplist->del(&se);
}

int ZSet::getRank(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict->find(key);
  if (!de) {
    return -1;
  }
  const double score = de->val;
  const ZSetEntry se(key, score);
  return skiplist->getRankofElement(&se);
}
}  // namespace zset
}  // namespace redis_simple
