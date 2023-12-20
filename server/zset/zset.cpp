#include "zset.h"

#include <cassert>

namespace redis_simple {
namespace zset {
ZSet::ZSet()
    : dict(in_memory::Dict<std::string, double>::Init()),
      skiplist(std::make_unique<
               in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>(
          in_memory::Skiplist<const ZSetEntry*, Comparator,
                              Destructor>::InitSkiplistLevel,
          Comparator(), Destructor())){};

void ZSet::addOrUpdate(const std::string& key, const double score) const {
  in_memory::Dict<std::string, double>::DictEntry* de = dict->Find(key);
  const ZSetEntry* ze = new ZSetEntry(key, score);
  if (de) {
    printf("update %s's val from %f to %f\n", key.c_str(), de->val, score);
    if (de->val == score) return;
    const ZSetEntry old(key, de->val);
    bool r = skiplist->Update(&old, ze);
    assert(r);
    de->val = score;
  } else {
    printf("insert %s %f\n", key.c_str(), score);
    assert(dict->Add(key, score));
    const ZSetEntry* inserted = skiplist->Insert(ze);
    assert(inserted);
  }
  min_key = std::min(min_key, key);
  max_key = std::max(max_key, key);
}

bool ZSet::remove(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict->Find(key);
  if (!de) {
    return false;
  }
  const double score = de->val;
  const ZSetEntry ze(key, score);
  assert(dict->Delete(key));
  return skiplist->Delete(&ze);
}

int ZSet::getRank(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict->Find(key);
  if (!de) {
    return -1;
  }
  const double score = de->val;
  const ZSetEntry ze(key, score);
  return skiplist->GetRankofKey(&ze);
}

std::vector<const ZSet::ZSetEntry*> ZSet::rangeByRank(
    const RangeByRankSpec* spec) const {
  const auto* skiplist_spec = toSkiplistRangeByRankSpec(spec);
  const std::vector<const ZSet::ZSetEntry*>& keys =
      spec->reverse ? skiplist->RevRangeByRank(skiplist_spec)
                    : skiplist->RangeByRank(skiplist_spec);
  freeSkiplistRangeByRankSpec(skiplist_spec);
  return keys;
}

std::vector<const ZSet::ZSetEntry*> ZSet::rangeByScore(
    const RangeByScoreSpec* spec) const {
  const auto* skiplist_spec = toSkiplistRangeByKeySpec(spec);
  const std::vector<const ZSet::ZSetEntry*>& keys =
      spec->reverse ? skiplist->RevRangeByKey(skiplist_spec)
                    : skiplist->RangeByKey(skiplist_spec);
  freeSkiplistRangeByKeySpec(skiplist_spec);
  return keys;
}

const in_memory::Skiplist<const ZSet::ZSetEntry*, ZSet::Comparator,
                          ZSet::Destructor>::SkiplistRangeByRankSpec*
ZSet::toSkiplistRangeByRankSpec(const RangeByRankSpec* spec) const {
  auto skiplist_spec =
      new in_memory::Skiplist<const ZSetEntry*, Comparator,
                              Destructor>::SkiplistRangeByRankSpec();
  skiplist_spec->min = spec->min;
  skiplist_spec->max = spec->max;
  skiplist_spec->minex = spec->minex;
  skiplist_spec->maxex = spec->maxex;
  if (spec->limit) {
    in_memory::Skiplist<const ZSetEntry*, Comparator,
                        Destructor>::SkiplistLimitSpec* limit =
        new in_memory::Skiplist<const ZSetEntry*, Comparator,
                                Destructor>::SkiplistLimitSpec();
    limit->offset = spec->limit->offset, limit->count = spec->limit->count,
    skiplist_spec->limit = limit;
  }
  return skiplist_spec;
}

const in_memory::Skiplist<const ZSet::ZSetEntry*, ZSet::Comparator,
                          ZSet::Destructor>::SkiplistRangeByKeySpec*
ZSet::toSkiplistRangeByKeySpec(const RangeByScoreSpec* spec) const {
  in_memory::Skiplist<const ZSetEntry*, Comparator,
                      Destructor>::SkiplistLimitSpec* limit = nullptr;
  if (spec->limit) {
    limit = new in_memory::Skiplist<const ZSetEntry*, Comparator,
                                    Destructor>::SkiplistLimitSpec();
    limit->offset = spec->limit->offset, limit->count = spec->limit->count;
  }

  /* if min score exclusive, set the zset entry key to be the max_key to exclude
   * all keys with the same score */
  const ZSetEntry* min_entry =
      new ZSetEntry(spec->minex ? max_key : min_key, spec->min);
  /* if max score exclusive, set the zset entry key to be the min_key to exclude
   * all keys with the same score */
  const ZSetEntry* max_entry =
      new ZSetEntry(spec->maxex ? min_key : max_key, spec->max);
  auto skiplist_spec =
      new in_memory::Skiplist<const ZSetEntry*, Comparator,
                              Destructor>::SkiplistRangeByKeySpec(min_entry,
                                                                  spec->minex,
                                                                  max_entry,
                                                                  spec->maxex,
                                                                  limit);
  return skiplist_spec;
}

void ZSet::freeSkiplistRangeByRankSpec(
    const in_memory::Skiplist<const ZSet::ZSetEntry*, ZSet::Comparator,
                              ZSet::Destructor>::SkiplistRangeByRankSpec*
        skiplist_spec) const {
  delete skiplist_spec->limit;
  delete skiplist_spec;
}

void ZSet::freeSkiplistRangeByKeySpec(
    const in_memory::Skiplist<const ZSet::ZSetEntry*, ZSet::Comparator,
                              ZSet::Destructor>::SkiplistRangeByKeySpec*
        skiplist_spec) const {
  delete skiplist_spec->limit;
  delete skiplist_spec->min;
  delete skiplist_spec->max;
  delete skiplist_spec;
}
}  // namespace zset
}  // namespace redis_simple
