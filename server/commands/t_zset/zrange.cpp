#include "server/commands/t_zset/zrange.h"

#include <limits>
#include <memory>
#include <optional>

#include "server/client.h"
#include "server/reply/reply.h"
#include "server/reply_utils/reply_utils.h"
#include "storage/zset/zset.h"
#include "utils/string_utils.h"

namespace redis_simple::command::t_zset {
namespace {
constexpr char kFlagByScore[] = "BYSCORE";
constexpr char kFlagLimit[] = "LIMIT";
constexpr char kFlagReverse[] = "REV";
constexpr char kFlagWithScores[] = "WITHSCORES";
constexpr char kMaxVal[] = "+inf";
constexpr char kMinVal[] = "-inf";

bool FlaggedByScore(const std::vector<std::string>& args);
bool ValidateRangeOptions(const std::vector<std::string>& args);
int ParseRangeToRankSpec(const std::vector<std::string>& args,
                         zset::RangeByRankSpec* spec);
int ParseRankRange(const std::string& start, const std::string& end,
                   zset::RangeByRankSpec* spec);
int ParseRangeTerm(const std::string& term, long* dst);
int ParseRangeToScoreSpec(const std::vector<std::string>& args,
                          zset::RangeByScoreSpec* spec);
int ParseScoreRange(const std::string& start, const std::string& end,
                    zset::RangeByScoreSpec* spec);
int ParseScoreTerm(const std::string& term, double* dst);
int ParseLimitOffsetAndCount(const std::vector<std::string>& args,
                             const std::unique_ptr<zset::LimitSpec>& spec);
bool IsReverse(const std::vector<std::string>& args);
bool IsWithScores(const std::vector<std::string>& args);
std::optional<std::string> EncodeZRangeReply(const zset::ZSetEntryList& result,
                                             bool with_scores);
int RangeByRank(Client* client, const std::vector<std::string>& args,
                zset::ZSetEntryList* result);
int RangeByScore(Client* client, const std::vector<std::string>& args,
                 zset::ZSetEntryList* result);

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

bool ValidateRangeOptions(const std::vector<std::string>& args) {
  bool has_by_score = false;
  bool has_limit = false;
  bool has_reverse = false;
  bool has_with_scores = false;
  for (size_t i = 3; i < args.size(); ++i) {
    std::string upper = args[i];
    utils::ToUppercase(upper);
    if (upper == kFlagByScore) {
      if (has_by_score) {
        return false;
      }
      has_by_score = true;
    } else if (upper == kFlagReverse) {
      if (has_reverse) {
        return false;
      }
      has_reverse = true;
    } else if (upper == kFlagWithScores) {
      if (has_with_scores) {
        return false;
      }
      has_with_scores = true;
    } else if (upper == kFlagLimit) {
      if (has_limit || i + 2 >= args.size()) {
        return false;
      }
      has_limit = true;
      i += 2;
    } else {
      return false;
    }
  }
  return true;
}

int ParseRangeToRankSpec(const std::vector<std::string>& args,
                         zset::RangeByRankSpec* const spec) {
  if (args.size() < 3 || !ValidateRangeOptions(args)) {
    return -1;
  }
  const std::string& start = args[1];
  const std::string& end = args[2];
  if (ParseRankRange(start, end, spec) < 0) {
    return -1;
  }
  spec->limit = std::make_unique<zset::LimitSpec>();
  if (ParseLimitOffsetAndCount(args, spec->limit) < 0) {
    return -1;
  }
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
  if (term.empty() || (dst == nullptr)) {
    return -1;
  }
  if (term == kMinVal) {
    *dst = 0;
  } else if (term == kMaxVal) {
    *dst = std::numeric_limits<long>::max();
  } else if (term[0] == '(') {
    try {
      *dst = std::stol(term.substr(1));
    } catch (const std::exception&) {
      return -1;
    }
  } else {
    try {
      *dst = std::stol(term);
    } catch (const std::exception&) {
      return -1;
    }
  }
  return 0;
}

int ParseRangeToScoreSpec(const std::vector<std::string>& args,
                          zset::RangeByScoreSpec* const spec) {
  if (args.size() < 3 || !ValidateRangeOptions(args)) {
    return -1;
  }
  const std::string& start = args[1];
  const std::string& end = args[2];
  if (ParseScoreRange(start, end, spec) < 0) {
    return -1;
  }
  spec->limit = std::make_unique<zset::LimitSpec>();
  if (ParseLimitOffsetAndCount(args, spec->limit) < 0) {
    return -1;
  }
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
  if (term.empty() || (dst == nullptr)) {
    return -1;
  }
  if (term == kMinVal) {
    *dst = -std::numeric_limits<double>::infinity();
  } else if (term == kMaxVal) {
    *dst = std::numeric_limits<double>::infinity();
  } else if (term[0] == '(') {
    try {
      *dst = std::stod(term.substr(1));
    } catch (const std::exception&) {
      return -1;
    }
  } else {
    try {
      *dst = std::stod(term);
    } catch (const std::exception&) {
      return -1;
    }
  }
  return 0;
}

int ParseLimitOffsetAndCount(const std::vector<std::string>& args,
                             const std::unique_ptr<zset::LimitSpec>& spec) {
  // LIMIT is optional; when absent, the zset implementation uses an unbounded
  // range.
  int i = 3;
  for (; i < args.size(); ++i) {
    std::string upper = args[i];
    utils::ToUppercase(upper);
    if (upper == kFlagLimit) {
      break;
    }
  }
  if (i == args.size()) {
    spec->offset = 0, spec->count = -1;
    return 0;
  }
  if (i + 2 >= args.size()) {
    return -1;
  }
  try {
    spec->offset = std::stol(args[i + 1]);
  } catch (const std::exception&) {
    return -1;
  }
  try {
    spec->count = std::stol(args[i + 2]);
  } catch (const std::exception&) {
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

bool IsWithScores(const std::vector<std::string>& args) {
  for (int i = 3; i < args.size(); ++i) {
    std::string upper = args[i];
    utils::ToUppercase(upper);
    if (upper == kFlagWithScores) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> EncodeZRangeReply(const zset::ZSetEntryList& result,
                                             bool with_scores) {
  if (!with_scores) {
    auto to_string = [](const zset::ZSetEntry* const& entry) {
      return entry->key;
    };
    return reply_utils::EncodeList<const zset::ZSetEntry*, to_string>(result);
  }
  std::vector<std::string> encoded_elements;
  encoded_elements.reserve(result.size() * 2);
  for (const auto* entry : result) {
    encoded_elements.push_back(reply::FromBulkString(entry->key));
    encoded_elements.push_back(reply::FromFloat(entry->score));
  }
  try {
    return reply::FromArray(encoded_elements);
  } catch (const std::exception& e) {
    RS_LOG_DEBUG("catch exception while encoding zrange reply: %s", e.what());
    return std::nullopt;
  }
}
}  // namespace

void ExecuteZRange(Client* const client) {
  const auto& args = client->Args();
  zset::ZSetEntryList range_entries;
  int status = 0;
  if (FlaggedByScore(args)) {
    status = RangeByScore(client, args, &range_entries);
  } else {
    status = RangeByRank(client, args, &range_entries);
  }
  if (status < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  const auto reply = EncodeZRangeReply(range_entries, IsWithScores(args));
  if (reply.has_value()) {
    client->AddReply(*reply);
  } else {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
  }
}

namespace {

int RangeByRank(Client* const client, const std::vector<std::string>& args,
                zset::ZSetEntryList* result) {
  zset::RangeByRankSpec spec;
  if (ParseRangeToRankSpec(args, &spec) < 0) {
    RS_LOG_DEBUG("invalid arguments for zrange rank\n");
    return -1;
  }
  if (auto* redis_db = client->Db()) {
    const auto& key = args[0];
    const auto* obj = redis_db->LookupKey(key);
    if (obj == nullptr) {
      return 0;
    }
    if (obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
      RS_LOG_DEBUG("incorrect value type\n");
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
    RS_LOG_DEBUG("db unavailable\n");
    return -1;
  }
  return 0;
}

int RangeByScore(Client* const client, const std::vector<std::string>& args,
                 zset::ZSetEntryList* result) {
  zset::RangeByScoreSpec spec;
  if (ParseRangeToScoreSpec(args, &spec) < 0) {
    RS_LOG_DEBUG("invalid arguments for zrange score\n");
    return -1;
  }
  if (auto* redis_db = client->Db()) {
    const auto& key = args[0];
    const auto* obj = redis_db->LookupKey(key);
    if (obj == nullptr) {
      return 0;
    }
    if (obj->Encoding() != db::RedisObject::ObjEncoding::kZSet) {
      RS_LOG_DEBUG("incorrect value type\n");
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
    RS_LOG_DEBUG("db unavailable\n");
    return -1;
  }
  return 0;
}
}  // namespace
}  // namespace redis_simple::command::t_zset
