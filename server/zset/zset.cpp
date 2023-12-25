#include "zset.h"

#include <cassert>

namespace redis_simple {
namespace zset {
ZSet::ZSet()
    : dict_(in_memory::Dict<std::string, double>::Init()),
      skiplist_(std::make_unique<
                in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>(
          in_memory::Skiplist<const ZSetEntry*, Comparator,
                              Destructor>::InitSkiplistLevel,
          Comparator(), Destructor())){};

void ZSet::AddOrUpdate(const std::string& key, const double score) const {
  in_memory::Dict<std::string, double>::DictEntry* de = dict_->Find(key);
  const ZSetEntry* ze = new ZSetEntry(key, score);
  if (de) {
    printf("update %s's val from %f to %f\n", key.c_str(), de->val, score);
    if (de->val == score) return;
    const ZSetEntry old(key, de->val);
    bool r = skiplist_->Update(&old, ze);
    assert(r);
    de->val = score;
  } else {
    printf("insert %s %f\n", key.c_str(), score);
    assert(dict_->Add(key, score));
    const ZSetEntry* inserted = skiplist_->Insert(ze);
    assert(inserted);
  }
  if (!min_key_.has_value() || key < min_key_.value()) {
    min_key_.emplace(key);
  }
  if (!max_key_.has_value() || key > max_key_.value()) {
    max_key_.emplace(key);
  }
}

bool ZSet::Remove(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict_->Find(key);
  if (!de) {
    return false;
  }
  const double score = de->val;
  const ZSetEntry ze(key, score);
  assert(dict_->Delete(key));
  return skiplist_->Delete(&ze);
}

int ZSet::GetRankOfKey(const std::string& key) const {
  const in_memory::Dict<std::string, double>::DictEntry* de = dict_->Find(key);
  if (!de) {
    return -1;
  }
  const double score = de->val;
  const ZSetEntry ze(key, score);
  return skiplist_->GetRankofKey(&ze);
}

const std::vector<const ZSet::ZSetEntry*> ZSet::RangeByRank(
    const RangeByRankSpec* spec) const {
  const auto* skiplist_spec = ToSkiplistRangeByRankSpec(spec);
  const std::vector<const ZSet::ZSetEntry*>& keys =
      spec->reverse ? skiplist_->RevRangeByRank(skiplist_spec)
                    : skiplist_->RangeByRank(skiplist_spec);
  FreeSkiplistRangeByRankSpec(skiplist_spec);
  return keys;
}

const std::vector<const ZSet::ZSetEntry*> ZSet::RangeByScore(
    const RangeByScoreSpec* spec) const {
  const auto* skiplist_spec = ToSkiplistRangeByKeySpec(spec);
  const std::vector<const ZSet::ZSetEntry*>& keys =
      spec->reverse ? skiplist_->RevRangeByKey(skiplist_spec)
                    : skiplist_->RangeByKey(skiplist_spec);
  FreeSkiplistRangeByKeySpec(skiplist_spec);
  return keys;
}

long ZSet::Count(const RangeByScoreSpec* spec) const {
  const auto* skiplist_spec = ToSkiplistRangeByKeySpec(spec);
  const long count = skiplist_->Count(skiplist_spec);
  FreeSkiplistRangeByKeySpec(skiplist_spec);
  return count == -1 ? 0 : count;
}

const in_memory::Skiplist<const ZSet::ZSetEntry*, ZSet::Comparator,
                          ZSet::Destructor>::SkiplistRangeByRankSpec*
ZSet::ToSkiplistRangeByRankSpec(const RangeByRankSpec* spec) const {
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
ZSet::ToSkiplistRangeByKeySpec(const RangeByScoreSpec* spec) const {
  in_memory::Skiplist<const ZSetEntry*, Comparator,
                      Destructor>::SkiplistLimitSpec* limit = nullptr;
  if (spec->limit) {
    limit = new in_memory::Skiplist<const ZSetEntry*, Comparator,
                                    Destructor>::SkiplistLimitSpec();
    limit->offset = spec->limit->offset, limit->count = spec->limit->count;
  }

  /* if min score exclusive, set the zset entry key to be the max_key to exclude
   * all keys with the same score */
  const ZSetEntry* min_entry = new ZSetEntry(
      spec->minex ? max_key_.value() : min_key_.value(), spec->min);
  /* if max score exclusive, set the zset entry key to be the min_key to exclude
   * all keys with the same score */
  const ZSetEntry* max_entry = new ZSetEntry(
      spec->maxex ? min_key_.value() : max_key_.value(), spec->max);
  auto skiplist_spec =
      new in_memory::Skiplist<const ZSetEntry*, Comparator,
                              Destructor>::SkiplistRangeByKeySpec(min_entry,
                                                                  spec->minex,
                                                                  max_entry,
                                                                  spec->maxex,
                                                                  limit);
  return skiplist_spec;
}

void ZSet::FreeSkiplistRangeByRankSpec(
    const in_memory::Skiplist<const ZSet::ZSetEntry*, ZSet::Comparator,
                              ZSet::Destructor>::SkiplistRangeByRankSpec*
        skiplist_spec) const {
  delete skiplist_spec->limit;
  delete skiplist_spec;
}

void ZSet::FreeSkiplistRangeByKeySpec(
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
