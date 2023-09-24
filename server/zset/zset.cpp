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
  const ZSetEntry* ze = new ZSetEntry(key, score);
  if (de) {
    printf("update %s's val from %f to %f\n", key.c_str(), de->val, score);
    if (de->val == score) return;
    const ZSetEntry old(key, de->val);
    bool r = skiplist->update(&old, ze);
    assert(r);
    de->val = score;
  } else {
    printf("insert %s %f\n", key.c_str(), score);
    assert(dict->add(key, score));
    const ZSetEntry* inserted = skiplist->insert(ze);
    assert(inserted);
  }
}

bool ZSet::remove(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict->find(key);
  if (!de) {
    return false;
  }
  const double score = de->val;
  const ZSetEntry ze(key, score);
  assert(dict->del(key));
  return skiplist->del(&ze);
}

int ZSet::getRank(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict->find(key);
  if (!de) {
    return -1;
  }
  const double score = de->val;
  const ZSetEntry ze(key, score);
  return skiplist->getRankofElement(&ze);
}
}  // namespace zset
}  // namespace redis_simple
