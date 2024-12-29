#include "server/zset/zset_skiplist.h"

namespace redis_simple {
namespace zset {
ZSetSkiplist::ZSetSkiplist()
    : dict_(in_memory::Dict<std::string, double>::Init()),
      skiplist_(std::make_unique<
                in_memory::Skiplist<const ZSetEntry*, Comparator, Destructor>>(
          in_memory::InitSkiplistLevel, Comparator(), Destructor())){};

bool ZSetSkiplist::InsertOrUpdate(const std::string& key, const double score) {
  const std::optional<double>& opt = dict_->Get(key);
  if (opt.has_value() && opt.value() == score) {
    /* if the key exists and there is no change in score, do nothing. */
    return false;
  }
  dict_->Set(key, score);
  const ZSetEntry* ze = new ZSetEntry(key, score);
  bool inserted = false;
  if (opt.has_value()) {
    /* update the score */
    printf("update %s's val from %f to %f\n", key.c_str(), opt.value(), score);
    const ZSetEntry old(key, opt.value());
    assert(skiplist_->Update(&old, ze));
  } else {
    /* insert a new key */
    printf("insert %s %f\n", key.c_str(), score);
    const ZSetEntry* inserted_entry = skiplist_->Insert(ze);
    assert(inserted_entry);
    inserted = true;
  }
  /* update min and max key */
  if (!min_key_.has_value() || key < min_key_.value()) {
    min_key_.emplace(key);
  }
  if (!max_key_.has_value() || key > max_key_.value()) {
    max_key_.emplace(key);
  }
  return inserted;
}

bool ZSetSkiplist::Delete(const std::string& key) {
  const std::optional<double>& opt = dict_->Get(key);
  if (!opt.has_value()) {
    return false;
  }
  const double score = opt.value();
  const ZSetEntry ze(key, score);
  assert(dict_->Delete(key));
  return skiplist_->Delete(&ze);
}

std::optional<size_t> ZSetSkiplist::GetRankOfKey(const std::string& key) const {
  const std::optional<double>& opt = dict_->Get(key);
  if (!opt.has_value()) {
    return std::nullopt;
  }
  const double score = opt.value();
  const ZSetEntry ze(key, score);
  return skiplist_->FindRankofKey(&ze);
}

std::vector<const ZSetEntry*> ZSetSkiplist::RangeByRank(
    const RangeByRankSpec* spec) const {
  const auto* skiplist_spec = ToSkiplistRangeByRankSpec(spec);
  const std::vector<const ZSetEntry*>& keys =
      spec->reverse ? skiplist_->RevRangeByRank(skiplist_spec)
                    : skiplist_->RangeByRank(skiplist_spec);
  FreeSkiplistRangeByRankSpec(skiplist_spec);
  return keys;
}

std::vector<const ZSetEntry*> ZSetSkiplist::RangeByScore(
    const RangeByScoreSpec* spec) const {
  const auto* skiplist_spec = ToSkiplistRangeByKeySpec(spec);
  const std::vector<const ZSetEntry*>& keys =
      spec->reverse ? skiplist_->RevRangeByKey(skiplist_spec)
                    : skiplist_->RangeByKey(skiplist_spec);
  FreeSkiplistRangeByKeySpec(skiplist_spec);
  return keys;
}

size_t ZSetSkiplist::Count(const RangeByScoreSpec* spec) const {
  const auto* skiplist_spec = ToSkiplistRangeByKeySpec(spec);
  const size_t count = skiplist_->Count(skiplist_spec);
  FreeSkiplistRangeByKeySpec(skiplist_spec);
  return count;
}

const in_memory::Skiplist<const ZSetEntry*, ZSetSkiplist::Comparator,
                          ZSetSkiplist::Destructor>::SkiplistRangeByRankSpec*
ZSetSkiplist::ToSkiplistRangeByRankSpec(const RangeByRankSpec* spec) const {
  auto skiplist_spec =
      new in_memory::Skiplist<const ZSetEntry*, Comparator,
                              Destructor>::SkiplistRangeByRankSpec();
  /* use overflow to check if the index is still negative after rebase */
  if ((spec->min < 0 && spec->min + Size() > Size()) ||
      (spec->max < 0 && spec->max + Size() > Size())) {
    /* the index is still negative after rebase */
    return nullptr;
  }
  skiplist_spec->min = spec->min < 0 ? (spec->min + Size()) : spec->min;
  skiplist_spec->max = spec->max < 0 ? (spec->max + Size()) : spec->max;
  skiplist_spec->minex = spec->minex;
  skiplist_spec->maxex = spec->maxex;
  if (spec->limit) {
    auto* limit = new in_memory::Skiplist<const ZSetEntry*, Comparator,
                                          Destructor>::SkiplistLimitSpec();
    limit->offset = spec->limit->offset, limit->count = spec->limit->count,
    skiplist_spec->limit = limit;
  }
  return skiplist_spec;
}

const in_memory::Skiplist<const ZSetEntry*, ZSetSkiplist::Comparator,
                          ZSetSkiplist::Destructor>::SkiplistRangeByKeySpec*
ZSetSkiplist::ToSkiplistRangeByKeySpec(const RangeByScoreSpec* spec) const {
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

void ZSetSkiplist::FreeSkiplistRangeByRankSpec(
    const in_memory::Skiplist<const ZSetEntry*, ZSetSkiplist::Comparator,
                              ZSetSkiplist::Destructor>::
        SkiplistRangeByRankSpec* skiplist_spec) const {
  if (skiplist_spec) {
    if (skiplist_spec->limit) {
      delete skiplist_spec->limit;
    }
    delete skiplist_spec;
  }
}

void ZSetSkiplist::FreeSkiplistRangeByKeySpec(
    const in_memory::Skiplist<const ZSetEntry*, ZSetSkiplist::Comparator,
                              ZSetSkiplist::Destructor>::SkiplistRangeByKeySpec*
        skiplist_spec) const {
  if (skiplist_spec) {
    if (skiplist_spec->limit) {
      delete skiplist_spec->limit;
    }
    delete skiplist_spec->min;
    delete skiplist_spec->max;
    delete skiplist_spec;
  }
}
}  // namespace zset
}  // namespace redis_simple
