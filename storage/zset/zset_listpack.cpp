#include "storage/zset/zset_listpack.h"

#include <cassert>

#include "memory/listpack.h"
#include "utils/float_utils.h"

namespace redis_simple {
namespace zset {
ZSetListPack::ZSetListPack() : listpack_(new in_memory::ListPack()) {}

bool ZSetListPack::InsertOrUpdate(const std::string& key, const double score) {
  ssize_t key_idx = listpack_->FindAndSkip(key, 1);
  bool inserted = key_idx < 0;
  if (key_idx >= 0) {
    ssize_t score_idx = listpack_->Next(key_idx);
    double curscore = GetScore(score_idx);
    if (curscore == score) return false;
    DeleteKeyScorePair(key_idx);
  }
  const std::string& score_str = utils::FloatToString(score);
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    size_t len = 0;
    const std::optional<std::string>& opt_str = listpack_->Get(idx);
    if (!opt_str.has_value()) continue;
    const std::string& ele = opt_str.value();
    ssize_t score_idx = listpack_->Next(idx);
    double ele_score = GetScore(score_idx);
    if (score < ele_score || (score == ele_score && key < ele)) {
      assert(listpack_->Insert(idx, key));
      idx = listpack_->Next(idx);
      assert(listpack_->Insert(idx, score_str));
      return inserted;
    }
    idx = listpack_->Next(score_idx);
  }
  assert(listpack_->Append(key));
  assert(listpack_->Append(score_str));
  return inserted;
}

bool ZSetListPack::Delete(const std::string& key) {
  ssize_t idx = listpack_->FindAndSkip(key, 1);
  if (idx < 0) return false;
  DeleteKeyScorePair(idx);
  return true;
}

std::optional<double> ZSetListPack::GetScoreOfKey(
    const std::string& key) const {
  /* find the index of the key */
  ssize_t idx = listpack_->FindAndSkip(key, 1);
  if (idx < 0) return std::nullopt;
  /* score is the next element of the key */
  idx = listpack_->Next(idx);
  const std::optional<std::string>& score = listpack_->Get(idx);
  return std::stod(score.value_or("0"));
}

std::optional<size_t> ZSetListPack::GetRankOfKey(const std::string& key) const {
  ssize_t key_idx = listpack_->FindAndSkip(key, 1);
  if (key_idx < 0) return std::nullopt;
  ssize_t idx = listpack_->First();
  size_t rank = 0;
  while (idx != key_idx) {
    idx = listpack_->Next(idx);
    /* skip the element score entry */
    idx = listpack_->Next(idx);
    ++rank;
  }
  return rank;
}

std::vector<const ZSetEntry*> ZSetListPack::RangeByRank(
    const RangeByRankSpec* spec) const {
  /* turn negative index to positive */
  RangeByRankSpec rank_spec;
  rank_spec.min = spec->min < 0 ? (spec->min + Size()) : spec->min;
  rank_spec.max = spec->max < 0 ? (spec->max + Size()) : spec->max;
  rank_spec.minex = spec->minex;
  rank_spec.maxex = spec->maxex;
  if (spec->limit) {
    rank_spec.limit =
        std::make_unique<LimitSpec>(spec->limit->offset, spec->limit->count);
  }
  if (!ValidateRangeRankSpec(&rank_spec)) return {};
  return spec->reverse ? RevRangeByRankUtil(&rank_spec)
                       : RangeByRankUtil(&rank_spec);
}

std::vector<const ZSetEntry*> ZSetListPack::RangeByScore(
    const RangeByScoreSpec* spec) const {
  if (!ValidateRangeScoreSpec(spec)) return {};
  return spec->reverse ? RevRangeByScoreUtil(spec) : RangeByScoreUtil(spec);
}

size_t ZSetListPack::Count(const RangeByScoreSpec* spec) const {
  ssize_t idx = listpack_->First();
  size_t count = 0;
  while (idx != -1) {
    size_t score_idx = listpack_->Next(idx);
    const std::optional<std::string>& opt_score = listpack_->Get(score_idx);
    if (opt_score.has_value() && IsInRange(opt_score.value(), spec)) {
      ++count;
    } else if (opt_score.has_value() && !LessOrEqual(opt_score.value(), spec)) {
      break;
    }
    /* proceed to the next key score pair */
    idx = listpack_->Next(idx);
    if (idx < 0) break;
    idx = listpack_->Next(idx);
  }
  return count;
}

void ZSetListPack::DeleteKeyScorePair(size_t idx) {
  /* delete the key */
  listpack_->Delete(idx);
  /* delete the score */
  listpack_->Delete(idx);
}

/*
 * Get the score of the key at the given index.
 */
double ZSetListPack::GetScore(size_t idx) {
  size_t len = 0;
  const std::optional<std::string>& opt_str = listpack_->Get(idx);
  return std::stod(opt_str.value_or("0"));
}

std::vector<const ZSetEntry*> ZSetListPack::RangeByRankUtil(
    const RangeByRankSpec* spec) const {
  ssize_t count = spec->limit ? spec->limit->count : -1;
  if (count == 0) return {};
  ssize_t idx = listpack_->First();
  std::vector<const ZSetEntry*> keys;
  size_t rank = 0, start = spec->minex ? spec->min + 1 : spec->min,
         end = spec->maxex ? spec->max : spec->max + 1,
         offset = spec->limit ? spec->limit->offset : 0;
  while (idx != -1 && rank < end) {
    if (rank >= start) {
      const std::optional<std::string>& opt_key = listpack_->Get(idx);
      ssize_t score_idx = listpack_->Next(idx);
      const std::optional<std::string>& opt_score = listpack_->Get(score_idx);
      if (opt_key.has_value() && opt_score.has_value() && rank >= offset) {
        keys.push_back(
            new ZSetEntry(opt_key.value(), std::stod(opt_score.value())));
        if (count >= 0 && keys.size() >= count) break;
      }
      idx = listpack_->Next(score_idx);
    } else {
      /* skip the key score pair */
      idx = listpack_->Next(idx);
      idx = listpack_->Next(idx);
    }
    ++rank;
  }
  return keys;
}

std::vector<const ZSetEntry*> ZSetListPack::RevRangeByRankUtil(
    const RangeByRankSpec* spec) const {
  ssize_t count = spec->limit ? spec->limit->count : -1;
  if (count == 0) return {};
  ssize_t idx = listpack_->Last();
  std::vector<const ZSetEntry*> keys;
  size_t rank = 0, start = spec->minex ? spec->min + 1 : spec->min,
         end = spec->maxex ? spec->max : spec->max + 1,
         offset = spec->limit ? spec->limit->offset : 0;
  while (idx != -1 && rank < end) {
    if (rank >= start) {
      const std::optional<std::string>& opt_score = listpack_->Get(idx);
      ssize_t key_idx = listpack_->Prev(idx);
      const std::optional<std::string>& opt_key = listpack_->Get(key_idx);
      if (opt_key.has_value() && opt_score.has_value() && rank >= offset) {
        keys.push_back(
            new ZSetEntry(opt_key.value(), std::stod(opt_score.value())));
        if (count >= 0 && keys.size() >= count) break;
      }
      idx = listpack_->Prev(key_idx);
    } else {
      /* skip the key score pair */
      idx = listpack_->Prev(idx);
      idx = listpack_->Prev(idx);
    }
    ++rank;
  }
  return keys;
}

std::vector<const ZSetEntry*> ZSetListPack::RangeByScoreUtil(
    const RangeByScoreSpec* spec) const {
  ssize_t count = spec->limit ? spec->limit->count : -1;
  if (count == 0) return {};
  ssize_t key_idx = FindKeyGreaterOrEqual(spec);
  if (key_idx < 0) return {};
  ssize_t score_idx = listpack_->Next(key_idx);
  std::vector<const ZSetEntry*> keys;
  size_t i = 0, offset = spec->limit ? spec->limit->offset : 0;
  while (key_idx != -1) {
    const std::optional<std::string>& score_opt = listpack_->Get(score_idx);
    if (score_opt.has_value() && IsInRange(score_opt.value(), spec) &&
        i >= offset) {
      const std::optional<std::string>& key_opt = listpack_->Get(key_idx);
      double score = std::stod(score_opt.value());
      keys.push_back(new ZSetEntry(key_opt.value(), score));
      if (count >= 0 && keys.size() == count) break;
    }
    key_idx = listpack_->Next(score_idx);
    if (key_idx < 0) break;
    score_idx = listpack_->Next(key_idx);
    ++i;
  }
  return keys;
}

std::vector<const ZSetEntry*> ZSetListPack::RevRangeByScoreUtil(
    const RangeByScoreSpec* spec) const {
  ssize_t count = spec->limit ? spec->limit->count : -1;
  if (count == 0) return {};
  ssize_t key_idx = FindKeyLessOrEqual(spec);
  if (key_idx < 0) return {};
  ssize_t score_idx = listpack_->Next(key_idx);
  std::vector<const ZSetEntry*> keys;
  size_t i = 0, offset = spec->limit ? spec->limit->offset : 0;
  while (key_idx != -1) {
    const std::optional<std::string>& score_opt = listpack_->Get(score_idx);
    if (score_opt.has_value() && IsInRange(score_opt.value(), spec) &&
        i >= offset) {
      const std::optional<std::string>& key_opt = listpack_->Get(key_idx);
      double score = std::stod(score_opt.value());
      keys.push_back(new ZSetEntry(key_opt.value(), score));
      if (count >= 0 && keys.size() == count) break;
    }
    score_idx = listpack_->Prev(key_idx);
    if (score_idx < 0) break;
    key_idx = listpack_->Prev(score_idx);
    ++i;
  }
  return keys;
}

ssize_t ZSetListPack::FindKeyGreaterOrEqual(
    const RangeByScoreSpec* spec) const {
  double min_score = spec->min;
  ssize_t key_idx = listpack_->First();
  if (key_idx < 0) return -1;
  /* score is the next element of the key */
  ssize_t score_idx = listpack_->Next(key_idx);
  while (key_idx != -1) {
    const std::optional<std::string>& score_opt = listpack_->Get(score_idx);
    if (score_opt.has_value()) {
      double score = std::stod(score_opt.value());
      if ((spec->minex && score > min_score) ||
          (!spec->minex && score >= min_score)) {
        return key_idx;
      }
    }
    key_idx = listpack_->Next(score_idx);
    if (key_idx < 0) break;
    score_idx = listpack_->Next(key_idx);
  }
  return -1;
}

ssize_t ZSetListPack::FindKeyLessOrEqual(const RangeByScoreSpec* spec) const {
  double max_score = spec->max;
  ssize_t score_idx = listpack_->Last();
  if (score_idx < 0) return -1;
  /* key is the previous element of the score */
  ssize_t key_idx = listpack_->Prev(score_idx);
  while (key_idx != -1) {
    const std::optional<std::string>& score_opt = listpack_->Get(score_idx);
    if (score_opt.has_value()) {
      double score = std::stod(score_opt.value());
      if ((spec->maxex && score < max_score) ||
          (!spec->maxex && score <= max_score)) {
        return key_idx;
      }
    }
    score_idx = listpack_->Prev(key_idx);
    if (score_idx < 0) break;
    key_idx = listpack_->Prev(score_idx);
  }
  return -1;
}

bool ZSetListPack::ValidateRangeRankSpec(const RangeByRankSpec* spec) {
  return spec && spec->min >= 0 && spec->max >= 0 &&
         ((!spec->minex && !spec->maxex && spec->min <= spec->max) ||
          spec->min < spec->max);
}

bool ZSetListPack::ValidateRangeScoreSpec(const RangeByScoreSpec* spec) {
  return spec && ((!spec->minex && !spec->maxex && spec->min <= spec->max) ||
                  spec->min < spec->max);
}

bool ZSetListPack::IsInRange(const std::string& score,
                             const RangeByScoreSpec* spec) {
  double score_val = std::stod(score);
  return (spec->minex ? score_val > spec->min : score_val >= spec->min) &&
         (spec->maxex ? score_val < spec->max : score_val <= spec->max);
}

bool ZSetListPack::LessOrEqual(const std::string& score,
                               const RangeByScoreSpec* spec) {
  double score_val = std::stod(score);
  return spec->maxex ? score_val < spec->max : score_val <= spec->max;
}
}  // namespace zset
}  // namespace redis_simple
