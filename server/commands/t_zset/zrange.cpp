#include "zrange.h"

#include <limits>

#include "server/client.h"
#include "server/zset/zset.h"
#include "utils/string_utils.h"

namespace redis_simple {
namespace command {
namespace t_zset {
namespace {
const std::string& flagByScore = "BYSCORE";
const std::string& flagLimit = "LIMIT";
const std::string& flagReverse = "REV";
const std::string& flagWithScores = "WITHSCORES";

bool FlaggedByScore(const std::vector<std::string>& args);
std::unique_ptr<const zset::ZSet::RangeByRankSpec> ParseRangeByRankSpec(
    const std::vector<std::string>& args);
int ParseRange(const std::string& start, const std::string& end,
               zset::ZSet::RangeByRankSpec* spec);
int ParseRangeTerm(const std::string& term, long* dst);
std::unique_ptr<const zset::ZSet::RangeByScoreSpec> ParseRangeByScoreSpec(
    const std::vector<std::string>& args);
int ParseScoreRange(const std::string& start, const std::string& end,
                    zset::ZSet::RangeByScoreSpec* spec);
int ParseScoreTerm(const std::string& term, double* dst);
int ParseLimitOffsetAndCount(const std::vector<std::string>& args, long* offset,
                             long* count);
bool IsReverse(const std::vector<std::string>& args);
const db::RedisObj* GetRedisObj(std::shared_ptr<const db::RedisDb> db,
                                const std::string& key);

bool FlaggedByScore(const std::vector<std::string>& args) {
  /* start searching at the 3rd index(0-based). Rankes before are key, start and
   * end offsets */
  for (int i = 3; i < args.size(); ++i) {
    std::string upper;
    std::transform(args[i].begin(), args[i].end(), upper.begin(), toupper);
    if (upper == flagByScore) {
      return true;
    }
  }
  return false;
}

std::unique_ptr<const zset::ZSet::RangeByRankSpec> ParseRangeByRankSpec(
    const std::vector<std::string>& args) {
  if (args.size() < 3) {
    return nullptr;
  }
  const std::string &start = args[2], &end = args[3];
  zset::ZSet::RangeByRankSpec* spec = new zset::ZSet::RangeByRankSpec();
  /* parse range */
  if (ParseRange(start, end, spec) < 0) {
    return nullptr;
  }
  spec->limit = std::make_unique<zset::ZSet::LimitSpec>();
  /* parse limit */
  ParseLimitOffsetAndCount(args, &(spec->limit->offset), &(spec->limit->count));
  /* parse reverse */
  spec->reverse = IsReverse(args);
  return std::unique_ptr<const zset::ZSet::RangeByRankSpec>(spec);
}

int ParseRange(const std::string& start, const std::string& end,
               zset::ZSet::RangeByRankSpec* spec) {
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

int ParseRangeTerm(const std::string& term, long* dst) {
  if (term == "+inf") {
    *dst = std::numeric_limits<long>::infinity();
  } else if (term == "-inf") {
    *dst = -std::numeric_limits<long>::infinity();
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

std::unique_ptr<const zset::ZSet::RangeByScoreSpec> ParseRangeByScoreSpec(
    const std::vector<std::string>& args) {
  if (args.size() < 3) {
    return nullptr;
  }
  const std::string &start = args[2], &end = args[3];
  zset::ZSet::RangeByScoreSpec* spec = new zset::ZSet::RangeByScoreSpec();
  /* parse score range */
  if (ParseScoreRange(start, end, spec) < 0) {
    return nullptr;
  }
  spec->limit = std::make_unique<zset::ZSet::LimitSpec>();
  /* parse limit */
  ParseLimitOffsetAndCount(args, &(spec->limit->offset), &(spec->limit->count));
  /* parse reverse */
  spec->reverse = IsReverse(args);
  return std::unique_ptr<const zset::ZSet::RangeByScoreSpec>(spec);
}

int ParseScoreRange(const std::string& start, const std::string& end,
                    zset::ZSet::RangeByScoreSpec* spec) {
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

int ParseScoreTerm(const std::string& term, double* dst) {
  if (term == "+inf") {
    *dst = std::numeric_limits<double>::infinity();
  } else if (term == "-inf") {
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

int ParseLimitOffsetAndCount(const std::vector<std::string>& args, long* offset,
                             long* count) {
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
  if (i >= args.size()) {
    /* limit flag not found or missing offset count info, set to default */
    *offset = 0, *count = -1;
    return 0;
  }
  try {
    *offset = std::stol(args[i + 1]);
  } catch (std::invalid_argument& e) {
    return -1;
  }
  try {
    *count = std::stol(args[i + 2]);
  } catch (std::invalid_argument& e) {
    return -1;
  }
  return 0;
}

bool IsReverse(const std::vector<std::string>& args) {
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

const db::RedisObj* GetRedisObj(std::shared_ptr<const db::RedisDb> db,
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
}  // namespace

void ZRangeCommand::Exec(Client* const client) const {
  const std::vector<std::string>& args = client->getArgs();
  std::vector<const zset::ZSet::ZSetEntry*> result;
  if (FlaggedByScore(args)) {
  } else {
    GenericRangeByRankSpec(client, args, &result);
  }
}

int ZRangeCommand::GenericRangeByRankSpec(
    Client* const client, const std::vector<std::string>& args,
    std::vector<const zset::ZSet::ZSetEntry*>* result) const {
  std::unique_ptr<const zset::ZSet::RangeByRankSpec> spec =
      ParseRangeByRankSpec(args);
  if (!spec) {
    printf("invalid arguments for zrange\n");
    return -1;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->getDb().lock()) {
    const std::string& key = args[0];
    const db::RedisObj* obj = GetRedisObj(db, key);
    if (!obj) {
      return -1;
    }
    try {
      const zset::ZSet* const zset = obj->ZSet();
      *result = zset->rangeByRank(spec.get());
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

int ZRangeCommand::GenericRangeByScoreSpec(
    Client* const client, const std::vector<std::string>& args,
    std::vector<const zset::ZSet::ZSetEntry*>* result) const {
  std::unique_ptr<const zset::ZSet::RangeByScoreSpec> spec =
      ParseRangeByScoreSpec(args);
  if (!spec) {
    printf("invalid arguments for zrange\n");
    return -1;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->getDb().lock()) {
    const std::string& key = args[0];
    const db::RedisObj* obj = GetRedisObj(db, key);
    if (!obj) {
      return -1;
    }
    try {
      const zset::ZSet* const zset = obj->ZSet();
      *result = zset->rangeByScore(spec.get());
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
