#include "zset.h"

#include "storage/zset/zset_listpack.h"
#include "storage/zset/zset_skiplist.h"

namespace redis_simple {
namespace zset {
ZSet::ZSet()
    : encoding_(ZSetEncodingType::ListPack),
      storage_(std::make_unique<ZSetListPack>()) {}

/*
 * Insert a new element with score or update the score of an existing
 * element. Return true if the element is newly inserted.
 */
bool ZSet::InsertOrUpdate(const std::string& key, const double score) {
  bool inserted = storage_->InsertOrUpdate(key, score);
  if (encoding_ == ZSetEncodingType::ListPack && inserted &&
      storage_->Size() > ListPackMaxEntries) {
    ConvertAndExpand();
  }
  return inserted;
}

bool ZSet::Delete(const std::string& key) { return storage_->Delete(key); }

std::optional<double> ZSet::GetScoreOfKey(const std::string& key) const {
  return storage_->GetScoreOfKey(key);
}

std::optional<size_t> ZSet::GetRankOfKey(const std::string& key) const {
  return storage_->GetRankOfKey(key);
}

const std::vector<const ZSetEntry*> ZSet::RangeByRank(
    const RangeByRankSpec* spec) const {
  return storage_->RangeByRank(spec);
}

const std::vector<const ZSetEntry*> ZSet::RangeByScore(
    const RangeByScoreSpec* spec) const {
  return storage_->RangeByScore(spec);
}

/*
 * Count number of elements within the range of the score.
 */
size_t ZSet::Count(const RangeByScoreSpec* spec) const {
  return storage_->Count(spec);
}

void ZSet::ConvertAndExpand() {
  assert(encoding_ == ZSetEncodingType::ListPack);
  encoding_ = ZSetEncodingType::Skiplist;
  if (!storage_) {
    storage_ = std::make_unique<ZSetSkiplist>();
    return;
  }
  ZSetStorage* zset_skiplist = new ZSetSkiplist();
  const RangeByRankSpec& spec = {
      .min = 0,
      .max = static_cast<long>(storage_->Size()),
  };
  const std::vector<const ZSetEntry*>& entries = storage_->RangeByRank(&spec);
  for (const ZSetEntry* entry : entries) {
    zset_skiplist->InsertOrUpdate(entry->key, entry->score);
  }
  storage_.reset(zset_skiplist);
}
}  // namespace zset
}  // namespace redis_simple
