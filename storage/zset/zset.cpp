#include "zset.h"

#include "storage/zset/zset_listpack.h"
#include "storage/zset/zset_skiplist.h"

namespace redis_simple::zset {
ZSet::ZSet()
    : encoding_(Encoding::kListPack),
      storage_(std::make_unique<ZSetListPack>()) {}

/*
 * Insert a new element with score or update the score of an existing
 * element. Return true if the element is newly inserted.
 */
bool ZSet::InsertOrUpdate(const std::string& key, double score) {
  if (encoding_ == Encoding::kListPack &&
      key.size() > kListPackMaxElementLength) {
    ConvertAndExpand();
  }
  bool inserted = storage_->InsertOrUpdate(key, score);
  if (ShouldConvertToSkiplist(key, inserted)) {
    ConvertAndExpand();
  }
  return inserted;
}

bool ZSet::Delete(const std::string& key) { return storage_->Delete(key); }

std::optional<double> ZSet::Score(const std::string& key) const {
  return storage_->Score(key);
}

std::optional<size_t> ZSet::Rank(const std::string& key) const {
  return storage_->Rank(key);
}

ZSetEntryList ZSet::RangeByRank(const RangeByRankSpec* spec) const {
  return storage_->RangeByRank(spec);
}

ZSetEntryList ZSet::RangeByScore(const RangeByScoreSpec* spec) const {
  return storage_->RangeByScore(spec);
}

/*
 * Count number of elements within the range of the score.
 */
size_t ZSet::Count(const RangeByScoreSpec* spec) const {
  return storage_->Count(spec);
}

enum ZSet::Encoding ZSet::Encoding() const { return encoding_; }

bool ZSet::ShouldConvertToSkiplist(const std::string& key,
                                   bool inserted) const {
  return encoding_ == Encoding::kListPack &&
         ((inserted && storage_->Size() > kListPackMaxEntries) ||
          key.size() > kListPackMaxElementLength);
}

void ZSet::ConvertAndExpand() {
  assert(encoding_ == Encoding::kListPack);
  encoding_ = Encoding::kSkiplist;
  if (!storage_) {
    storage_ = std::make_unique<ZSetSkiplist>();
    return;
  }
  auto zset_skiplist = std::make_unique<ZSetSkiplist>();
  RangeByRankSpec spec;
  spec.min = 0;
  spec.max = static_cast<long>(storage_->Size()) - 1;
  spec.minex = false;
  spec.maxex = false;
  spec.limit = nullptr;
  spec.reverse = false;
  const auto entries = storage_->RangeByRank(&spec);
  for (const auto* entry : entries) {
    zset_skiplist->InsertOrUpdate(entry->key, entry->score);
  }
  storage_ = std::move(zset_skiplist);
}
}  // namespace redis_simple::zset
