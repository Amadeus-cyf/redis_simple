#include "storage/zset/zset_listpack.h"

#include <cassert>
#include <limits>

#include "memory/listpack.h"
#include "utils/float_utils.h"

namespace redis_simple::zset {
ZSetListPack::ZSetListPack()
    : listpack_(std::make_unique<in_memory::ListPack>()) {}

bool ZSetListPack::InsertOrUpdate(const std::string& key, double score) {
  ssize_t key_idx = listpack_->FindAndSkip(key, 1);
  bool inserted = key_idx < 0;
  if (key_idx >= 0) {
    ssize_t score_idx = listpack_->Next(key_idx);
    double curscore = ScoreAt(score_idx);
    if (curscore == score) {
      return false;
    }
    DeleteKeyScorePair(key_idx);
  }
  const std::string& score_str = utils::FloatToString(score);
  ssize_t idx = listpack_->First();
  while (idx != -1) {
    const auto string_result = listpack_->Get(idx);
    if (!string_result.has_value()) {
      continue;
    }
    const std::string& ele = *string_result;
    ssize_t score_idx = listpack_->Next(idx);
    double ele_score = ScoreAt(score_idx);
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
  if (idx < 0) {
    return false;
  }
  DeleteKeyScorePair(idx);
  return true;
}

std::optional<double> ZSetListPack::Score(const std::string& key) const {
  // Find the index of the key.
  ssize_t idx = listpack_->FindAndSkip(key, 1);
  if (idx < 0) {
    return std::nullopt;
  }
  // Score is the next element of the key.
  idx = listpack_->Next(idx);
  const auto score = listpack_->Get(idx);
  if (!score.has_value()) {
    return std::nullopt;
  }
  return std::stod(*score);
}

std::optional<size_t> ZSetListPack::Rank(const std::string& key) const {
  ssize_t key_idx = listpack_->FindAndSkip(key, 1);
  if (key_idx < 0) {
    return std::nullopt;
  }
  ssize_t idx = listpack_->First();
  size_t rank = 0;
  while (idx != key_idx) {
    idx = listpack_->Next(idx);
    // Skip the element score entry.
    idx = listpack_->Next(idx);
    ++rank;
  }
  return rank;
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
    size_t score_idx = listpack_->Next(idx);
    const auto opt_score = listpack_->Get(score_idx);
    if (!opt_score.has_value()) {
      break;
    }
    if (IsInRange(*opt_score, spec)) {
      ++count;
    } else if (!LessOrEqual(*opt_score, spec)) {
      break;
    }
    // Proceed to the next key score pair.
    idx = listpack_->Next(idx);
    if (idx < 0) {
      break;
    }
    idx = listpack_->Next(idx);
  }
  return count;
}

void ZSetListPack::DeleteKeyScorePair(size_t idx) {
  // Delete the key.
  listpack_->Delete(idx);
  // Delete the score.
  listpack_->Delete(idx);
}

/*
 * Get the score of the key at the given index.
 */
double ZSetListPack::ScoreAt(size_t idx) {
  const auto string_result = listpack_->Get(idx);
  return string_result.has_value() ? std::stod(*string_result) : 0;
}

ZSetEntryList ZSetListPack::RangeByRankUtil(const RangeByRankSpec* spec) const {
  ssize_t count = spec->limit ? spec->limit->count : -1;
  if (count == 0) {
    return {};
  }
  ssize_t idx = listpack_->First();
  ZSetEntryList keys;
  size_t rank = 0;
  size_t start = spec->minex ? spec->min + 1 : spec->min;
  size_t end = spec->maxex ? spec->max : spec->max + 1;
  size_t offset = spec->limit ? spec->limit->offset : 0;
  while (idx != -1 && rank < end) {
    if (rank >= start) {
      const auto opt_key = listpack_->Get(idx);
      ssize_t score_idx = listpack_->Next(idx);
      const auto opt_score = listpack_->Get(score_idx);
      if (opt_key.has_value() && opt_score.has_value() && rank >= offset) {
        keys.push_back(AddRangeResult(*opt_key, std::stod(*opt_score)));
        if (count >= 0 && keys.size() >= count) {
          break;
        }
      }
      idx = listpack_->Next(score_idx);
    } else {
      // Skip the key score pair.
      idx = listpack_->Next(idx);
      idx = listpack_->Next(idx);
    }
    ++rank;
  }
  return keys;
}

ZSetEntryList ZSetListPack::RevRangeByRankUtil(
    const RangeByRankSpec* spec) const {
  ssize_t count = spec->limit ? spec->limit->count : -1;
  if (count == 0) {
    return {};
  }
  ssize_t idx = listpack_->Last();
  ZSetEntryList keys;
  size_t rank = 0;
  size_t start = spec->minex ? spec->min + 1 : spec->min;
  size_t end = spec->maxex ? spec->max : spec->max + 1;
  size_t offset = spec->limit ? spec->limit->offset : 0;
  while (idx != -1 && rank < end) {
    if (rank >= start) {
      const auto opt_score = listpack_->Get(idx);
      ssize_t key_idx = listpack_->Prev(idx);
      const auto opt_key = listpack_->Get(key_idx);
      if (opt_key.has_value() && opt_score.has_value() && rank >= offset) {
        keys.push_back(AddRangeResult(*opt_key, std::stod(*opt_score)));
        if (count >= 0 && keys.size() >= count) {
          break;
        }
      }
      idx = listpack_->Prev(key_idx);
    } else {
      // Skip the key score pair.
      idx = listpack_->Prev(idx);
      idx = listpack_->Prev(idx);
    }
    ++rank;
  }
  return keys;
}

ZSetEntryList ZSetListPack::RangeByScoreUtil(
    const RangeByScoreSpec* spec) const {
  ssize_t count = spec->limit ? spec->limit->count : -1;
  if (count == 0) {
    return {};
  }
  ssize_t key_idx = FindKeyGreaterOrEqual(spec);
  if (key_idx < 0) {
    return {};
  }
  ssize_t score_idx = listpack_->Next(key_idx);
  ZSetEntryList keys;
  size_t i = 0;
  size_t offset = spec->limit ? spec->limit->offset : 0;
  while (key_idx != -1) {
    const auto score_opt = listpack_->Get(score_idx);
    if (score_opt.has_value() && IsInRange(*score_opt, spec) && i >= offset) {
      const auto key_opt = listpack_->Get(key_idx);
      if (key_opt.has_value()) {
        double score = std::stod(*score_opt);
        keys.push_back(AddRangeResult(*key_opt, score));
        if (count >= 0 && keys.size() == count) {
          break;
        }
      }
    }
    key_idx = listpack_->Next(score_idx);
    if (key_idx < 0) {
      break;
    }
    score_idx = listpack_->Next(key_idx);
    ++i;
  }
  return keys;
}

ZSetEntryList ZSetListPack::RevRangeByScoreUtil(
    const RangeByScoreSpec* spec) const {
  ssize_t count = spec->limit ? spec->limit->count : -1;
  if (count == 0) {
    return {};
  }
  ssize_t key_idx = FindKeyLessOrEqual(spec);
  if (key_idx < 0) {
    return {};
  }
  ssize_t score_idx = listpack_->Next(key_idx);
  ZSetEntryList keys;
  size_t i = 0;
  size_t offset = spec->limit ? spec->limit->offset : 0;
  while (key_idx != -1) {
    const auto score_opt = listpack_->Get(score_idx);
    if (score_opt.has_value() && IsInRange(*score_opt, spec) && i >= offset) {
      const auto key_opt = listpack_->Get(key_idx);
      if (key_opt.has_value()) {
        double score = std::stod(*score_opt);
        keys.push_back(AddRangeResult(*key_opt, score));
        if (count >= 0 && keys.size() == count) {
          break;
        }
      }
    }
    score_idx = listpack_->Prev(key_idx);
    if (score_idx < 0) {
      break;
    }
    key_idx = listpack_->Prev(score_idx);
    ++i;
  }
  return keys;
}

const ZSetEntry* ZSetListPack::AddRangeResult(const std::string& key,
                                              double score) const {
  range_cache_.push_back(std::make_unique<ZSetEntry>(key, score));
  return range_cache_.back().get();
}

ssize_t ZSetListPack::FindKeyGreaterOrEqual(
    const RangeByScoreSpec* spec) const {
  double min_score = spec->min;
  ssize_t key_idx = listpack_->First();
  if (key_idx < 0) {
    return -1;
  }
  // Score is the next element of the key.
  ssize_t score_idx = listpack_->Next(key_idx);
  while (key_idx != -1) {
    const auto score_opt = listpack_->Get(score_idx);
    if (score_opt.has_value()) {
      double score = std::stod(*score_opt);
      if ((spec->minex && score > min_score) ||
          (!spec->minex && score >= min_score)) {
        return key_idx;
      }
    }
    key_idx = listpack_->Next(score_idx);
    if (key_idx < 0) {
      break;
    }
    score_idx = listpack_->Next(key_idx);
  }
  return -1;
}

ssize_t ZSetListPack::FindKeyLessOrEqual(const RangeByScoreSpec* spec) const {
  double max_score = spec->max;
  ssize_t score_idx = listpack_->Last();
  if (score_idx < 0) {
    return -1;
  }
  // Key is the previous element of the score.
  ssize_t key_idx = listpack_->Prev(score_idx);
  while (key_idx != -1) {
    const auto score_opt = listpack_->Get(score_idx);
    if (score_opt.has_value()) {
      double score = std::stod(*score_opt);
      if ((spec->maxex && score < max_score) ||
          (!spec->maxex && score <= max_score)) {
        return key_idx;
      }
    }
    score_idx = listpack_->Prev(key_idx);
    if (score_idx < 0) {
      break;
    }
    key_idx = listpack_->Prev(score_idx);
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
}  // namespace redis_simple::zset
