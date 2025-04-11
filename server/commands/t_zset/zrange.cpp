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
const std::string& flagByScore = "BYSCORE";
const std::string& flagLimit = "LIMIT";
const std::string& flagReverse = "REV";
const std::string& flagWithScores = "WITHSCORES";
const std::string& maxVal = "+inf";
const std::string& minVal = "-inf";

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
const db::RedisObj* GetRedisObj(std::shared_ptr<const db::RedisDb> db,
                                const std::string& key);

bool FlaggedByScore(const std::vector<std::string>& args) {
  // Start searching at the 3rd index(0-based). The first 3 arguments specify
  // key, start and end offsets.
  for (int i = 3; i < args.size(); ++i) {
    std::string upper = args[i];
    std::transform(upper.begin(), upper.end(), upper.begin(), toupper);
    if (upper == flagByScore) {
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
  const std::string &start = args[1], &end = args[2];
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
  spec->minex = (start[0] == '(');
  spec->maxex = (end[0] == '(');
  return 0;
}

int ParseRangeTerm(const std::string& term, long* const dst) {
  if (term == minVal) {
    *dst = 0;
  } else if (term == maxVal) {
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
  const std::string &start = args[1], &end = args[2];
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
  spec->minex = (start[0] == '(');
  spec->maxex = (end[0] == '(');
  return 0;
}

int ParseScoreTerm(const std::string& term, double* const dst) {
  if (term == minVal) {
    *dst = -std::numeric_limits<double>::infinity();
  } else if (term == maxVal) {
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
  // Start searching at the 3rd index(0-based). The first 3 arguments specify
  // key, start and end offsets.
  int i = 3;
  for (; i < args.size(); ++i) {
    std::string upper;
    std::transform(args[i].begin(), args[i].end(), upper.begin(), toupper);
    if (upper == flagLimit) {
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
  // Start searching at the 3rd index(0-based). The first 3 arguments specify
  // key, start and end offsets.
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
  const std::vector<std::string>& args = client->CmdArgs();
  std::vector<const zset::ZSetEntry*> result;
  int r = 0;
  if (FlaggedByScore(args)) {
    r = RangeByScore(client, args, &result);
  } else {
    r = RangeByRank(client, args, &result);
  }
  if (r < 0) {
    client->AddReply(reply::FromInt64(reply::replyErr));
    return;
  }
  auto to_string = [](const zset::ZSetEntry* const& entry) {
    return entry->key;
  };
  const std::optional<const std::string>& reply =
      reply_utils::EncodeList<const zset::ZSetEntry*, to_string>(result);
  if (reply.has_value()) {
    client->AddReply(reply.value());
  } else {
    client->AddReply(reply::FromInt64(reply::replyErr));
  }
}

int ZRangeCommand::RangeByRank(
    Client* const client, const std::vector<std::string>& args,
    std::vector<const zset::ZSetEntry*>* result) const {
  zset::RangeByRankSpec spec;
  if (ParseRangeToRankSpec(args, &spec) < 0) {
    printf("invalid arguments for zrange rank\n");
    return -1;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    const std::string& key = std::move(args[0]);
    const db::RedisObj* obj = GetRedisObj(db, key);
    if (!obj) {
      return -1;
    }
    try {
      zset::ZSet* const zset = obj->ZSet();
      *result = zset->RangeByRank(&spec);
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
    std::vector<const zset::ZSetEntry*>* result) const {
  zset::RangeByScoreSpec spec;
  if (ParseRangeToScoreSpec(args, &spec) < 0) {
    printf("invalid arguments for zrange score\n");
    return -1;
  }
  if (std::shared_ptr<const db::RedisDb> db = client->DB().lock()) {
    const std::string& key = std::move(args[0]);
    const db::RedisObj* obj = GetRedisObj(db, key);
    if (!obj) {
      return -1;
    }
    try {
      const zset::ZSet* zset = obj->ZSet();
      *result = zset->RangeByScore(&spec);
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
