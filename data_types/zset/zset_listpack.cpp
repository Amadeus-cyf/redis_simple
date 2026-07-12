#include "data_types/zset/zset_listpack.h"

#include <cassert>
#include <limits>
#include <optional>

#include "memory/listpack.h"
#include "utils/float_utils.h"

namespace redis_simple::zset {
ZSetListPack::ZSetListPack()
    : listpack_(std::make_unique<in_memory::ListPack>()) {}

bool ZSetListPack::InsertOrUpdate(std::string_view key, double score) {
  ssize_t key_idx = listpack_->FindAndSkip(key, 1);
  bool inserted = key_idx < 0;
  if (key_idx >= 0) {
    const auto entry = EntryAt(key_idx);
    if (!entry.has_value()) {
      return false;
    }
    if (entry->score == score) {
      return false;
    }
    DeleteKeyScorePair(key_idx);
  }
  const std::string score_str = utils::FloatToString(score);
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    const auto entry = EntryAt(idx);
    if (!entry.has_value()) {
      break;
    }
    if (score < entry->score || (score == entry->score && key < entry->key)) {
      assert(listpack_->Insert(idx, key));
      idx = listpack_->Next(idx);
      assert(listpack_->Insert(idx, score_str));
      return inserted;
    }
    idx = NextKeyAfterScore(entry->score_index);
  }
  assert(listpack_->Append(key));
  assert(listpack_->Append(score_str));
  return inserted;
}

bool ZSetListPack::Delete(std::string_view key) {
  ssize_t idx = listpack_->FindAndSkip(key, 1);
  if (idx < 0) {
    return false;
  }
  DeleteKeyScorePair(idx);
  return true;
}

std::optional<double> ZSetListPack::Score(std::string_view key) const {
  ssize_t idx = listpack_->FindAndSkip(key, 1);
  if (idx < 0) {
    return std::nullopt;
  }
  const auto entry = EntryAt(idx);
  return entry.has_value() ? std::optional<double>(entry->score) : std::nullopt;
}

std::optional<size_t> ZSetListPack::Rank(std::string_view key) const {
  ssize_t key_idx = listpack_->FindAndSkip(key, 1);
  if (key_idx < 0) {
    return std::nullopt;
  }
  ssize_t idx = listpack_->First();
  size_t rank = 0;
  while (idx != -1 && idx != key_idx) {
    const auto entry = EntryAt(idx);
    if (!entry.has_value()) {
      return std::nullopt;
    }
    idx = NextKeyAfterScore(entry->score_index);
    ++rank;
  }
  return idx == key_idx ? std::optional<size_t>(rank) : std::nullopt;
}

ZSetEntryList ZSetListPack::RangeByRank(const RangeByRankSpec* spec) const {
  range_cache_.clear();
  // Turn negative index to positive.
  const size_t size = Size();
  if (size > static_cast<size_t>(std::numeric_limits<long>::max())) {
    return {};
  }
  const long zset_size = static_cast<long>(size);
  RangeByRankSpec rank_spec;
  rank_spec.min = spec->min < 0 ? (spec->min + zset_size) : spec->min;
  rank_spec.max = spec->max < 0 ? (spec->max + zset_size) : spec->max;
  rank_spec.minex = spec->minex;
  rank_spec.maxex = spec->maxex;
  if (spec->limit) {
    rank_spec.limit =
        std::make_unique<LimitSpec>(spec->limit->offset, spec->limit->count);
  }
  if (!ValidateRangeRankSpec(&rank_spec)) {
    return {};
  }
  return spec->reverse ? RevRangeByRankUtil(&rank_spec)
                       : RangeByRankUtil(&rank_spec);
}

ZSetEntryList ZSetListPack::RangeByScore(const RangeByScoreSpec* spec) const {
  range_cache_.clear();
  if (!ValidateRangeScoreSpec(spec)) {
    return {};
  }
  return spec->reverse ? RevRangeByScoreUtil(spec) : RangeByScoreUtil(spec);
}

size_t ZSetListPack::Count(const RangeByScoreSpec* spec) const {
  ssize_t idx = listpack_->First();
  size_t count = 0;
  while (idx != -1) {
    const auto entry = EntryAt(idx);
    if (!entry.has_value()) {
      break;
    }
    if (IsInRange(entry->score, spec)) {
      ++count;
    } else if (!LessOrEqual(entry->score, spec)) {
      break;
    }
    idx = NextKeyAfterScore(entry->score_index);
  }
  return count;
}

void ZSetListPack::DeleteKeyScorePair(size_t idx) {
  listpack_->Delete(idx);
  listpack_->Delete(idx);
}

std::optional<std::string_view> ZSetListPack::ValueAt(size_t idx) const {
  size_t len = 0;
  const auto* data = listpack_->Get(idx, &len);
  if (data == nullptr) {
    return std::nullopt;
  }
  return std::string_view(reinterpret_cast<const char*>(data), len);
}

std::optional<ZSetListPack::EntryView> ZSetListPack::EntryAt(
    ssize_t key_idx) const {
  if (key_idx < 0) {
    return std::nullopt;
  }
  const auto key = ValueAt(static_cast<size_t>(key_idx));
  if (!key.has_value()) {
    return std::nullopt;
  }
  const ssize_t score_idx = listpack_->Next(static_cast<size_t>(key_idx));
  if (score_idx < 0) {
    return std::nullopt;
  }
  const auto score = ScoreAt(static_cast<size_t>(score_idx));
  if (!score.has_value()) {
    return std::nullopt;
  }
  return EntryView{*key, *score, key_idx, score_idx};
}

std::optional<double> ZSetListPack::ScoreAt(size_t idx) const {
  const auto integer_score = listpack_->IntegerAt(idx);
  if (integer_score.has_value()) {
    return static_cast<double>(*integer_score);
  }
  const auto string_result = listpack_->Get(idx);
  if (!string_result.has_value()) {
    return std::nullopt;
  }
  return std::stod(*string_result);
}

ZSetEntryList ZSetListPack::RangeByRankUtil(const RangeByRankSpec* spec) const {
  const std::optional<size_t> count =
      spec->limit ? spec->limit->count : std::nullopt;
  if (count.has_value() && *count == 0) {
    return {};
  }
  ssize_t idx = listpack_->First();
  ZSetEntryList keys;
  size_t rank = 0;
  size_t start = spec->minex ? spec->min + 1 : spec->min;
  size_t end = spec->maxex ? spec->max : spec->max + 1;
  size_t offset = spec->limit ? spec->limit->offset : 0;
  while (idx != -1 && rank < end) {
    const auto entry = EntryAt(idx);
    if (!entry.has_value()) {
      break;
    }
    if (rank >= start) {
      if (rank >= offset) {
        keys.push_back(AddRangeResult(entry->key, entry->score));
        if (count.has_value() && keys.size() >= *count) {
          break;
        }
      }
    }
    idx = NextKeyAfterScore(entry->score_index);
    ++rank;
  }
  return keys;
}

ZSetEntryList ZSetListPack::RevRangeByRankUtil(
    const RangeByRankSpec* spec) const {
  const std::optional<size_t> count =
      spec->limit ? spec->limit->count : std::nullopt;
  if (count.has_value() && *count == 0) {
    return {};
  }
  ssize_t idx = listpack_->Last();
  ZSetEntryList keys;
  size_t rank = 0;
  size_t start = spec->minex ? spec->min + 1 : spec->min;
  size_t end = spec->maxex ? spec->max : spec->max + 1;
  size_t offset = spec->limit ? spec->limit->offset : 0;
  while (idx != -1 && rank < end) {
    const ssize_t key_idx = listpack_->Prev(static_cast<size_t>(idx));
    const auto entry = EntryAt(key_idx);
    if (!entry.has_value()) {
      break;
    }
    if (rank >= start) {
      if (rank >= offset) {
        keys.push_back(AddRangeResult(entry->key, entry->score));
        if (count.has_value() && keys.size() >= *count) {
          break;
        }
      }
    }
    idx = PrevScoreBeforeKey(entry->key_index);
    ++rank;
  }
  return keys;
}

ZSetEntryList ZSetListPack::RangeByScoreUtil(
    const RangeByScoreSpec* spec) const {
  const std::optional<size_t> count =
      spec->limit ? spec->limit->count : std::nullopt;
  if (count.has_value() && *count == 0) {
    return {};
  }
  ssize_t key_idx = FindKeyGreaterOrEqual(spec);
  if (key_idx < 0) {
    return {};
  }
  ZSetEntryList keys;
  size_t i = 0;
  size_t offset = spec->limit ? spec->limit->offset : 0;
  while (key_idx != -1) {
    const auto entry = EntryAt(key_idx);
    if (!entry.has_value() || !LessOrEqual(entry->score, spec)) {
      break;
    }
    if (IsInRange(entry->score, spec) && i >= offset) {
      keys.push_back(AddRangeResult(entry->key, entry->score));
      if (count.has_value() && keys.size() == *count) {
        break;
      }
    }
    key_idx = NextKeyAfterScore(entry->score_index);
    ++i;
  }
  return keys;
}

ZSetEntryList ZSetListPack::RevRangeByScoreUtil(
    const RangeByScoreSpec* spec) const {
  const std::optional<size_t> count =
      spec->limit ? spec->limit->count : std::nullopt;
  if (count.has_value() && *count == 0) {
    return {};
  }
  ssize_t key_idx = FindKeyLessOrEqual(spec);
  if (key_idx < 0) {
    return {};
  }
  ZSetEntryList keys;
  size_t i = 0;
  size_t offset = spec->limit ? spec->limit->offset : 0;
  while (key_idx != -1) {
    const auto entry = EntryAt(key_idx);
    if (!entry.has_value() || !GreaterOrEqual(entry->score, spec)) {
      break;
    }
    if (IsInRange(entry->score, spec) && i >= offset) {
      keys.push_back(AddRangeResult(entry->key, entry->score));
      if (count.has_value() && keys.size() == *count) {
        break;
      }
    }
    key_idx = PrevKeyBeforeKey(entry->key_index);
    ++i;
  }
  return keys;
}

const ZSetEntry* ZSetListPack::AddRangeResult(std::string_view key,
                                              double score) const {
  range_cache_.push_back(std::make_unique<ZSetEntry>(key, score));
  return range_cache_.back().get();
}

ssize_t ZSetListPack::NextKeyAfterScore(ssize_t score_idx) const {
  return score_idx < 0 ? -1 : listpack_->Next(static_cast<size_t>(score_idx));
}

ssize_t ZSetListPack::PrevScoreBeforeKey(ssize_t key_idx) const {
  return key_idx < 0 ? -1 : listpack_->Prev(static_cast<size_t>(key_idx));
}

ssize_t ZSetListPack::PrevKeyBeforeKey(ssize_t key_idx) const {
  const ssize_t score_idx = PrevScoreBeforeKey(key_idx);
  return score_idx < 0 ? -1 : listpack_->Prev(static_cast<size_t>(score_idx));
}

ssize_t ZSetListPack::FindKeyGreaterOrEqual(
    const RangeByScoreSpec* spec) const {
  ssize_t key_idx = listpack_->First();
  if (key_idx < 0) {
    return -1;
  }
  while (key_idx != -1) {
    const auto entry = EntryAt(key_idx);
    if (!entry.has_value()) {
      break;
    }
    if (GreaterOrEqual(entry->score, spec)) {
      return key_idx;
    }
    key_idx = NextKeyAfterScore(entry->score_index);
  }
  return -1;
}

ssize_t ZSetListPack::FindKeyLessOrEqual(const RangeByScoreSpec* spec) const {
  ssize_t score_idx = listpack_->Last();
  if (score_idx < 0) {
    return -1;
  }
  ssize_t key_idx = listpack_->Prev(score_idx);
  while (key_idx != -1) {
    const auto entry = EntryAt(key_idx);
    if (!entry.has_value()) {
      break;
    }
    if (LessOrEqual(entry->score, spec)) {
      return key_idx;
    }
    key_idx = PrevKeyBeforeKey(entry->key_index);
  }
  return -1;
}

bool ZSetListPack::ValidateRangeRankSpec(const RangeByRankSpec* spec) {
  return (spec != nullptr) && spec->min >= 0 && spec->max >= 0 &&
         ((!spec->minex && !spec->maxex && spec->min <= spec->max) ||
          spec->min < spec->max);
}

bool ZSetListPack::ValidateRangeScoreSpec(const RangeByScoreSpec* spec) {
  return (spec != nullptr) &&
         ((!spec->minex && !spec->maxex && spec->min <= spec->max) ||
          spec->min < spec->max);
}

bool ZSetListPack::IsInRange(double score, const RangeByScoreSpec* spec) {
  return (spec->minex ? score > spec->min : score >= spec->min) &&
         (spec->maxex ? score < spec->max : score <= spec->max);
}

bool ZSetListPack::GreaterOrEqual(double score, const RangeByScoreSpec* spec) {
  return spec->minex ? score > spec->min : score >= spec->min;
}

bool ZSetListPack::LessOrEqual(double score, const RangeByScoreSpec* spec) {
  return spec->maxex ? score < spec->max : score <= spec->max;
}
}  // namespace redis_simple::zset
