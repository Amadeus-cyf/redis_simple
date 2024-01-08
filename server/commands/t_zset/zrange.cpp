#include "zrange.h"

#include <limits>
#include <memory>

#include "server/client.h"
#include "server/reply/reply.h"
#include "server/zset/zset.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace command {
namespace t_zset {
namespace {
static const std::string& flagByScore = "BYSCORE";
static const std::string& flagLimit = "LIMIT";
static const std::string& flagReverse = "REV";
static const std::string& flagWithScores = "WITHSCORES";
static const std::string& maxScore = "+inf";
static const std::string& minScore = "-inf";

static bool FlaggedByScore(const std::vector<std::string>& args);
std::unique_ptr<const zset::ZSet::RangeByRankSpec> ParseRangeToRankSpec(
    const std::vector<std::string>& args);
static int ParseRankRange(
    const std::string& start, const std::string& end,
    const std::unique_ptr<zset::ZSet::RangeByRankSpec>& spec);
static int ParseRangeTerm(const std::string& term, long* const dst);
std::unique_ptr<const zset::ZSet::RangeByScoreSpec> ParseRangeToScoreSpec(
    const std::vector<std::string>& args);
static int ParseScoreRange(
    const std::string& start, const std::string& end,
    const std::unique_ptr<zset::ZSet::RangeByScoreSpec>& spec);
static int ParseScoreTerm(const std::string& term, double* const dst);
static int ParseLimitOffsetAndCount(
    const std::vector<std::string>& args,
    const std::unique_ptr<zset::ZSet::LimitSpec>& spec);
static bool IsReverse(const std::vector<std::string>& args);
static const db::RedisObj* GetRedisObj(std::shared_ptr<const db::RedisDb> db,
                                       const std::string& key);
static std::optional<const std::string> Encode(
    const std::vector<const zset::ZSet::ZSetEntry*>& result);

static bool FlaggedByScore(const std::vector<std::string>& args) {
  /* start searching at the 3rd index(0-based). Arguments before are key, start
   * and end */
  for (int i = 3; i < args.size(); ++i) {
    std::string upper = args[i];
    std::transform(upper.begin(), upper.end(), upper.begin(), toupper);
    if (upper == flagByScore) {
      return true;
    }
  }
  return false;
}

static std::unique_ptr<const zset::ZSet::RangeByRankSpec> ParseRangeToRankSpec(
    const std::vector<std::string>& args) {
  if (args.size() < 3) {
    return nullptr;
  }
  const std::string &start = args[1], &end = args[2];
  std::unique_ptr<zset::ZSet::RangeByRankSpec> spec =
      std::make_unique<zset::ZSet::RangeByRankSpec>();
  /* parse range */
  if (ParseRankRange(start, end, spec) < 0) {
    return nullptr;
  }
  spec->limit = std::make_unique<zset::ZSet::LimitSpec>();
  /* parse limit */
  ParseLimitOffsetAndCount(args, spec->limit);
  /* parse reverse */
  spec->reverse = IsReverse(args);
  return spec;
}

static int ParseRankRange(
    const std::string& start, const std::string& end,
    const std::unique_ptr<zset::ZSet::RangeByRankSpec>& spec) {
  if (ParseRangeTerm(start, &(spec->min)) < 0) {
    return -1;
  }
  if (ParseRangeTerm(end, &(spec->max)) < 0) {
    return -1;
  }
  if (start[0] == '(') {
    spec->minex = true;
  }
  if (end[0] == '(') {
    spec->maxex = true;
  }
  return 0;
}

static int ParseRangeTerm(const std::string& term, long* const dst) {
  if (term == minScore) {
    *dst = 0;
  } else if (term == maxScore) {
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

static std::unique_ptr<const zset::ZSet::RangeByScoreSpec>
ParseRangeToScoreSpec(const std::vector<std::string>& args) {
  if (args.size() < 3) {
    return nullptr;
  }
  const std::string &start = args[1], &end = args[2];
  std::unique_ptr<zset::ZSet::RangeByScoreSpec> spec =
      std::make_unique<zset::ZSet::RangeByScoreSpec>();
  /* parse score range */
  if (ParseScoreRange(start, end, spec) < 0) {
    return nullptr;
  }
  spec->limit = std::make_unique<zset::ZSet::LimitSpec>();
  /* parse limit */
  ParseLimitOffsetAndCount(args, spec->limit);
  /* parse reverse */
  spec->reverse = IsReverse(args);
  return spec;
}

static int ParseScoreRange(
    const std::string& start, const std::string& end,
    const std::unique_ptr<zset::ZSet::RangeByScoreSpec>& spec) {
  if (ParseScoreTerm(start, &(spec->min)) < 0) {
    return -1;
  }
  if (ParseScoreTerm(end, &(spec->max)) < 0) {
    return -1;
  }
  if (start[0] == '(') {
    spec->minex = true;
  }
  if (end[0] == '(') {
    spec->maxex = true;
  }
  return 0;
}

static int ParseScoreTerm(const std::string& term, double* const dst) {
  if (term == minScore) {
    *dst = -std::numeric_limits<double>::infinity();
  } else if (term == maxScore) {
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

static int ParseLimitOffsetAndCount(
    const std::vector<std::string>& args,
    const std::unique_ptr<zset::ZSet::LimitSpec>& spec) {
  /* start searching at the 3rd index(0-based). Rankes before are key, start,
   * end offsets */
  int i = 3;
  for (; i < args.size(); ++i) {
    std::string upper;
    std::transform(args[i].begin(), args[i].end(), upper.begin(), toupper);
    if (upper == flagLimit) {
      break;
    }
  }
  if (i > args.size() - 3) {
    /* limit flag not found or missing offset count info, set to default */
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

static bool IsReverse(const std::vector<std::string>& args) {
  /* start searching at the 3rd index(0-based). Rankes before are key, start,
   * end offsets */
  for (int i = 3; i < args.size(); ++i) {
    std::string upper;
    std::transform(args[i].begin(), args[i].end(), upper.begin(), toupper);
    if (upper == flagReverse) {
      return true;
    }
  }
  return false;
}

static const db::RedisObj* GetRedisObj(std::shared_ptr<const db::RedisDb> db,
                                       const std::string& key) {
  const db::RedisObj* obj = db->LookupKey(key);
  if (!obj) {
    printf("key not found\n");
    return nullptr;
  }
  if (obj->Encoding() != db::RedisObj::ObjEncoding::objEncodingZSet) {
    printf("incorrect value type\n");
    return nullptr;
  }
  return obj;
}

static std::optional<const std::string> Encode(
    const std::vector<const zset::ZSet::ZSetEntry*>& result) {
  std::vector<std::string> array;
  for (const zset::ZSet::ZSetEntry* entry : result) {
    array.push_back(reply::FromBulkString(entry->key));
  }
  try {
    const std::string& reply = reply::FromArray(array);
    return std::optional<const std::string>(reply);
  } catch (const std::exception& e) {
    printf("catch exception while encoding the array %s", e.what());
    return std::nullopt;
  }
}
}  // namespace

void ZRangeCommand::Exec(Client* const client) const {
  const std::vector<std::string>& args = client->CmdArgs();
  std::vector<const zset::ZSet::ZSetEntry*> result;
  int r = 0;
  if (FlaggedByScore(args)) {
    r = RangeByScore(client, args, &result);
  } else {
    r = RangeByRank(client, args, &result);
  }
  if (r < 0) {
    client->AddReply(reply::FromInt64(reply::replyErr));
  }
  const std::optional<const std::string>& reply = Encode(result);
  if (reply.has_value()) {
    client->AddReply(reply.value());
  } else {
    client->AddReply(reply::FromInt64(reply::replyErr));
  }
}

int ZRangeCommand::RangeByRank(
    Client* const client, const std::vector<std::string>& args,
    std::vector<const zset::ZSet::ZSetEntry*>* result) const {
  std::unique_ptr<const zset::ZSet::RangeByRankSpec> spec =
      ParseRangeToRankSpec(args);
  if (!spec) {
    printf("invalid arguments for zrange rank\n");
    return -1;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    const std::string& key = args[0];
    const db::RedisObj* obj = GetRedisObj(db, key);
    if (!obj) {
      return -1;
    }
    try {
      const zset::ZSet* const zset = obj->ZSet();
      *result = zset->RangeByRank(spec.get());
    } catch (const std::exception& e) {
      printf("catch exception %s", e.what());
      return -1;
    }
  } else {
    printf("db pointer expired\n");
    return -1;
  }
  return 0;
}

int ZRangeCommand::RangeByScore(
    Client* const client, const std::vector<std::string>& args,
    std::vector<const zset::ZSet::ZSetEntry*>* result) const {
  std::unique_ptr<const zset::ZSet::RangeByScoreSpec> spec =
      ParseRangeToScoreSpec(args);
  if (!spec) {
    printf("invalid arguments for zrange score\n");
    return -1;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    const std::string& key = args[0];
    const db::RedisObj* obj = GetRedisObj(db, key);
    if (!obj) {
      return -1;
    }
    try {
      const zset::ZSet* const zset = obj->ZSet();
      *result = zset->RangeByScore(spec.get());
    } catch (const std::exception& e) {
      printf("catch exception %s", e.what());
      return -1;
    }
  } else {
    printf("db pointer expired\n");
    return -1;
  }
  return 0;
}
}  // namespace t_zset
}  // namespace command
}  // namespace redis_simple
