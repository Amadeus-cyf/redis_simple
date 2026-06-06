#include "server/commands/t_zset/zrange.h"

#include <limits>
#include <memory>

#include "server/client.h"
#include "server/reply/reply.h"
#include "server/reply_utils/reply_utils.h"
#include "storage/zset/zset.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace command {
namespace t_zset {
namespace {
const std::string& kFlagByScore = "BYSCORE";
const std::string& kFlagLimit = "LIMIT";
const std::string& kFlagReverse = "REV";
const std::string& kFlagWithScores = "WITHSCORES";
const std::string& kMaxVal = "+inf";
const std::string& kMinVal = "-inf";

bool FlaggedByScore(const std::vector<std::string>& args);
int ParseRangeToRankSpec(const std::vector<std::string>& args,
                         zset::RangeByRankSpec* const spec);
int ParseRankRange(const std::string& start, const std::string& end,
                   zset::RangeByRankSpec* const spec);
int ParseRangeTerm(const std::string& term, long* const dst);
int ParseRangeToScoreSpec(const std::vector<std::string>& args,
                          zset::RangeByScoreSpec* const spec);
int ParseScoreRange(const std::string& start, const std::string& end,
                    zset::RangeByScoreSpec* const spec);
int ParseScoreTerm(const std::string& term, double* const dst);
int ParseLimitOffsetAndCount(const std::vector<std::string>& args,
                             const std::unique_ptr<zset::LimitSpec>& spec);
bool IsReverse(const std::vector<std::string>& args);
const db::RedisObject* GetRedisObj(std::shared_ptr<const db::RedisDb> db,
                                   const std::string& key);

bool FlaggedByScore(const std::vector<std::string>& args) {
  // The command parser stores args as key/start/end/options, so options begin
  // after the first three entries.
  for (int i = 3; i < args.size(); ++i) {
    std::string upper = args[i];
    utils::ToUppercase(upper);
    if (upper == kFlagByScore) {
      return true;
    }
  }
  return false;
}

int ParseRangeToRankSpec(const std::vector<std::string>& args,
                         zset::RangeByRankSpec* const spec) {
  if (args.size() < 3) {
    return -1;
  }
  const std::string& start = args[1];
  const std::string& end = args[2];
  // Parse range.
  if (ParseRankRange(start, end, spec) < 0) {
    return -1;
  }
  spec->limit = std::make_unique<zset::LimitSpec>();
  // Parse limit.
  if (ParseLimitOffsetAndCount(args, spec->limit) < 0) {
    return -1;
  }
  // Parse reverse.
  spec->reverse = IsReverse(args);
  return 0;
}

int ParseRankRange(const std::string& start, const std::string& end,
                   zset::RangeByRankSpec* const spec) {
  if (ParseRangeTerm(start, &(spec->min)) < 0) {
    return -1;
  }
  if (ParseRangeTerm(end, &(spec->max)) < 0) {
    return -1;
  }
  // Redis uses a leading '(' to make a range endpoint exclusive.
  spec->minex = (start[0] == '(');
  spec->maxex = (end[0] == '(');
  return 0;
}

int ParseRangeTerm(const std::string& term, long* const dst) {
  if (term == kMinVal) {
    *dst = 0;
  } else if (term == kMaxVal) {
    *dst = std::numeric_limits<long>::max();
  } else if (term[0] == '(') {
    try {
      *dst = std::stol(term.substr(1));
    } catch (std::invalid_argument& e) {
      return -1;
    }
  } else {
    try {
      *dst = std::stol(term);
    } catch (std::invalid_argument& e) {
      return -1;
    }
  }
  return 0;
}

int ParseRangeToScoreSpec(const std::vector<std::string>& args,
                          zset::RangeByScoreSpec* const spec) {
  if (args.size() < 3) {
    return -1;
  }
  const std::string& start = args[1];
  const std::string& end = args[2];
  // Parse score range.
  if (ParseScoreRange(start, end, spec) < 0) {
    return -1;
  }
  spec->limit = std::make_unique<zset::LimitSpec>();
  // Parse limit.
  if (ParseLimitOffsetAndCount(args, spec->limit) < 0) {
    return -1;
  }
  // Parse reverse.
  spec->reverse = IsReverse(args);
  return 0;
}

int ParseScoreRange(const std::string& start, const std::string& end,
                    zset::RangeByScoreSpec* const spec) {
  if (ParseScoreTerm(start, &(spec->min)) < 0) {
    return -1;
  }
  if (ParseScoreTerm(end, &(spec->max)) < 0) {
    return -1;
  }
  // Redis uses a leading '(' to make a score endpoint exclusive.
  spec->minex = (start[0] == '(');
  spec->maxex = (end[0] == '(');
  return 0;
}

int ParseScoreTerm(const std::string& term, double* const dst) {
  if (term == kMinVal) {
    *dst = -std::numeric_limits<double>::infinity();
  } else if (term == kMaxVal) {
    *dst = std::numeric_limits<double>::infinity();
  } else if (term[0] == '(') {
    try {
      *dst = std::stod(term.substr(1));
    } catch (std::invalid_argument& e) {
      return -1;
    }
  } else {
    try {
      *dst = std::stod(term);
    } catch (std::invalid_argument& e) {
      return -1;
    }
  }
  return 0;
}

int ParseLimitOffsetAndCount(const std::vector<std::string>& args,
                             const std::unique_ptr<zset::LimitSpec>& spec) {
  // LIMIT is optional; missing or incomplete LIMIT falls back to the unbounded
  // range used by the zset implementation.
  int i = 3;
  for (; i < args.size(); ++i) {
    std::string upper = args[i];
    utils::ToUppercase(upper);
    if (upper == kFlagLimit) {
      break;
    }
  }
  if (i > args.size() - 3) {
    // Limit flag not found or missing offset count info, set to default.
    spec->offset = 0, spec->count = -1;
    return 0;
  }
  try {
    spec->offset = std::stol(args[i + 1]);
  } catch (std::invalid_argument& e) {
    return -1;
  }
  try {
    spec->count = std::stol(args[i + 2]);
  } catch (std::invalid_argument& e) {
    return -1;
  }
  return 0;
}

bool IsReverse(const std::vector<std::string>& args) {
  // REV is an option token, so it can appear after key/start/end.
  for (int i = 3; i < args.size(); ++i) {
    std::string upper = args[i];
    utils::ToUppercase(upper);
    if (upper == kFlagReverse) {
      return true;
    }
  }
  return false;
}

const db::RedisObject* GetRedisObj(std::shared_ptr<const db::RedisDb> db,
                                   const std::string& key) {
  const auto* obj = db->LookupKey(key);
  if (!obj) {
    RS_LOG_DEBUG("key not found\n");
    return nullptr;
  }
  if (obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
    RS_LOG_DEBUG("incorrect value type\n");
    return nullptr;
  }
  return obj;
}
}  // namespace

void ZRangeCommand::Exec(Client* const client) const {
  const auto& args = client->CmdArgs();
  std::vector<const zset::ZSetEntry*> result;
  int r = 0;
  if (FlaggedByScore(args)) {
    r = RangeByScore(client, args, &result);
  } else {
    r = RangeByRank(client, args, &result);
  }
  if (r < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  auto to_string = [](const zset::ZSetEntry* const& entry) {
    return entry->key;
  };
  const auto reply =
      reply_utils::EncodeList<const zset::ZSetEntry*, to_string>(result);
  if (reply.has_value()) {
    client->AddReply(reply.value());
  } else {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

int ZRangeCommand::RangeByRank(
    Client* const client, const std::vector<std::string>& args,
    std::vector<const zset::ZSetEntry*>* result) const {
  zset::RangeByRankSpec spec;
  if (ParseRangeToRankSpec(args, &spec) < 0) {
    RS_LOG_DEBUG("invalid arguments for zrange rank\n");
    return -1;
  }
  if (auto db = client->DB().lock()) {
    const auto& key = args[0];
    const auto* obj = GetRedisObj(db, key);
    if (!obj) {
      return -1;
    }
    try {
      auto* const zset = obj->ZSet();
      *result = zset->RangeByRank(&spec);
    } catch (const std::exception& e) {
      RS_LOG_DEBUG("catch exception %s", e.what());
      return -1;
    }
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    return -1;
  }
  return 0;
}

int ZRangeCommand::RangeByScore(
    Client* const client, const std::vector<std::string>& args,
    std::vector<const zset::ZSetEntry*>* result) const {
  zset::RangeByScoreSpec spec;
  if (ParseRangeToScoreSpec(args, &spec) < 0) {
    RS_LOG_DEBUG("invalid arguments for zrange score\n");
    return -1;
  }
  if (auto db = client->DB().lock()) {
    const auto& key = args[0];
    const auto* obj = GetRedisObj(db, key);
    if (!obj) {
      return -1;
    }
    try {
      const auto* zset = obj->ZSet();
      *result = zset->RangeByScore(&spec);
    } catch (const std::exception& e) {
      RS_LOG_DEBUG("catch exception %s", e.what());
      return -1;
    }
  } else {
    RS_LOG_DEBUG("db pointer expired\n");
    return -1;
  }
  return 0;
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
