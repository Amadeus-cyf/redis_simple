#include "storage/zset/zset_skiplist.h"

namespace redis_simple::zset {
ZSetSkiplist::ZSetSkiplist()
    : dict_(in_memory::Dict<std::string, double>::Create()),
      skiplist_(std::make_unique<SkiplistType>(in_memory::kInitSkiplistLevel,
                                               Comparator(), Destructor())) {}

bool ZSetSkiplist::InsertOrUpdate(const std::string& key, double score) {
  const auto result = dict_->Get(key);
  if (result.has_value() && *result == score) {
    // If the key exists and there is no change in score, do nothing.
    return false;
  }
  auto ze = std::make_unique<ZSetEntry>(key, score);
  bool inserted = false;
  if (result.has_value()) {
    // Update the score.
    const ZSetEntry old(key, *result);
    if (!skiplist_->Update(&old, ze.get())) {
      return false;
    }
  } else {
    // Insert a new key.
    const auto* inserted_entry = skiplist_->Insert(ze.get());
    if (inserted_entry == nullptr) {
      return false;
    }
    inserted = true;
  }
  ze.release();
  dict_->Set(key, score);
  // Update min and max key.
  if (!min_key_.has_value() || key < *min_key_) {
    min_key_.emplace(key);
  }
  if (!max_key_.has_value() || key > *max_key_) {
    max_key_.emplace(key);
  }
  return inserted;
}

bool ZSetSkiplist::Delete(const std::string& key) {
  const auto result = dict_->Get(key);
  if (!result.has_value()) {
    return false;
  }
  double score = *result;
  const ZSetEntry ze(key, score);
  assert(dict_->Delete(key));
  bool deleted = skiplist_->Delete(&ze);
  if (deleted && ((min_key_.has_value() && *min_key_ == key) ||
                  (max_key_.has_value() && *max_key_ == key))) {
    RecomputeMinMaxKeys();
  }
  return deleted;
}

std::optional<size_t> ZSetSkiplist::Rank(const std::string& key) const {
  const auto result = dict_->Get(key);
  if (!result.has_value()) {
    return std::nullopt;
  }
  double score = *result;
  const ZSetEntry ze(key, score);
  return skiplist_->FindRankOfKey(&ze);
}

ZSetEntryList ZSetSkiplist::RangeByRank(const RangeByRankSpec* spec) const {
  if (spec == nullptr) {
    return {};
  }
  const auto skiplist_spec = ToSkiplistRangeByRankSpec(spec);
  return spec->reverse ? skiplist_->RevRangeByRank(skiplist_spec.get())
                       : skiplist_->RangeByRank(skiplist_spec.get());
}

ZSetEntryList ZSetSkiplist::RangeByScore(const RangeByScoreSpec* spec) const {
  if ((spec == nullptr) || Size() == 0) {
    return {};
  }
  const auto skiplist_spec = ToSkiplistRangeByKeySpec(spec);
  return spec->reverse ? skiplist_->RevRangeByKey(skiplist_spec.get())
                       : skiplist_->RangeByKey(skiplist_spec.get());
}

size_t ZSetSkiplist::Count(const RangeByScoreSpec* spec) const {
  if ((spec == nullptr) || Size() == 0) {
    return 0;
  }
  const auto skiplist_spec = ToSkiplistRangeByKeySpec(spec);
  return skiplist_->Count(skiplist_spec.get());
}

ZSetSkiplist::RankSpecPtr ZSetSkiplist::ToSkiplistRangeByRankSpec(
    const RangeByRankSpec* spec) const {
  if (spec == nullptr) {
    return {nullptr};
  }
  auto skiplist_spec = RankSpecPtr(new SkiplistRangeByRankSpec());
  // Use overflow to check if the index is still negative after rebase.
  if ((spec->min < 0 && spec->min + Size() > Size()) ||
      (spec->max < 0 && spec->max + Size() > Size())) {
    // The index is still negative after rebase.
    return {nullptr};
  }
  skiplist_spec->min = spec->min < 0 ? (spec->min + Size()) : spec->min;
  skiplist_spec->max = spec->max < 0 ? (spec->max + Size()) : spec->max;
  skiplist_spec->minex = spec->minex;
  skiplist_spec->maxex = spec->maxex;
  if (spec->limit) {
    auto limit = std::make_unique<SkiplistLimitSpec>();
    limit->offset = spec->limit->offset;
    limit->count = spec->limit->count;
    skiplist_spec->limit = limit.release();
  }
  return skiplist_spec;
}

ZSetSkiplist::KeySpecPtr ZSetSkiplist::ToSkiplistRangeByKeySpec(
    const RangeByScoreSpec* spec) const {
  if ((spec == nullptr) || !min_key_.has_value() || !max_key_.has_value()) {
    return {nullptr};
  }
  std::unique_ptr<SkiplistLimitSpec> limit;
  if (spec->limit) {
    limit = std::make_unique<SkiplistLimitSpec>();
    limit->offset = spec->limit->offset;
    limit->count = spec->limit->count;
  }

  // If min score exclusive, set the zset entry key to be the max_key to exclude
  // all keys with the same score.
  auto min_entry = std::make_unique<ZSetEntry>(
      spec->minex ? *max_key_ : *min_key_, spec->min);
  // If max score exclusive, set the zset entry key to be the min_key to exclude
  // all keys with the same score.
  auto max_entry = std::make_unique<ZSetEntry>(
      spec->maxex ? *min_key_ : *max_key_, spec->max);
  auto skiplist_spec = KeySpecPtr(new SkiplistRangeByKeySpec(
      min_entry.release(), spec->minex, max_entry.release(), spec->maxex,
      limit.release()));
  return skiplist_spec;
}

void ZSetSkiplist::RecomputeMinMaxKeys() {
  min_key_.reset();
  max_key_.reset();
  auto it = in_memory::Dict<std::string, double>::Iterator(dict_.get());
  it.SeekToFirst();
  while (it.Valid()) {
    const std::string& key = it.Key();
    if (!min_key_.has_value() || key < *min_key_) {
      min_key_.emplace(key);
    }
    if (!max_key_.has_value() || key > *max_key_) {
      max_key_.emplace(key);
    }
    it.Next();
  }
}

void ZSetSkiplist::RankSpecDeleter::operator()(
    SkiplistRangeByRankSpec* spec) const {
  if (spec != nullptr) {
    delete spec->limit;
    delete spec;
  }
}

void ZSetSkiplist::KeySpecDeleter::operator()(
    SkiplistRangeByKeySpec* spec) const {
  if (spec != nullptr) {
    delete spec->limit;
    delete spec->min;
    delete spec->max;
    delete spec;
  }
}
}  // namespace redis_simple::zset
