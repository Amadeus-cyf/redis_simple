#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "data_types/zset/zset.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/reply/reply.h"
#include "utils/string_utils.h"

namespace redis_simple::command::zsets {
namespace {
using LimitSpec = ::redis_simple::zset::LimitSpec;
using RangeByRankSpec = ::redis_simple::zset::RangeByRankSpec;
using RangeByScoreSpec = ::redis_simple::zset::RangeByScoreSpec;
using ZSetEntry = ::redis_simple::zset::ZSetEntry;
using ZSetEntryList = ::redis_simple::zset::ZSetEntryList;

constexpr std::string_view kFlagByScore = "BYSCORE";
constexpr std::string_view kFlagLimit = "LIMIT";
constexpr std::string_view kFlagReverse = "REV";
constexpr std::string_view kFlagWithScores = "WITHSCORES";
constexpr std::string_view kMaxVal = "+inf";
constexpr std::string_view kMinVal = "-inf";
constexpr int kRangeSyntaxError = -1;
constexpr int kRangeWrongType = -2;
constexpr int kRangeDbUnavailable = -3;

bool FlaggedByScore(const CommandArgs& args);
bool ValidateRangeOptions(const CommandArgs& args);
int ParseRangeToRankSpec(const CommandArgs& args, RangeByRankSpec* spec);
int ParseRankRange(std::string_view start, std::string_view end,
                   RangeByRankSpec* spec);
int ParseRangeTerm(std::string_view term, long* dst);
int ParseRangeToScoreSpec(const CommandArgs& args, RangeByScoreSpec* spec);
int ParseScoreRange(std::string_view start, std::string_view end,
                    RangeByScoreSpec* spec);
int ParseScoreTerm(std::string_view term, double* dst);
int ParseLimitOffsetAndCount(const CommandArgs& args,
                             const std::unique_ptr<LimitSpec>& spec);
bool IsReverse(const CommandArgs& args);
bool IsWithScores(const CommandArgs& args);
std::optional<std::string> EncodeZRangeReply(const ZSetEntryList& result,
                                             bool with_scores);
std::optional<int64_t> ToReplyInteger(size_t value);
void AddRangeReply(Client* client, const CommandArgs& args, bool by_score);
int RangeByRank(Client* client, const CommandArgs& args, ZSetEntryList* result);
int RangeByScore(Client* client, const CommandArgs& args,
                 ZSetEntryList* result);

bool FlaggedByScore(const CommandArgs& args) {
  // The command parser stores args as key/start/end/options, so options begin
  // after the first three entries.
  for (size_t i = 3; i < args.size(); ++i) {
    std::string upper(args[i]);
    utils::ToUppercase(upper);
    if (std::string_view(upper) == kFlagByScore) {
      return true;
    }
  }
  return false;
}

bool ValidateRangeOptions(const CommandArgs& args) {
  bool has_by_score = false;
  bool has_limit = false;
  bool has_reverse = false;
  bool has_with_scores = false;
  for (size_t i = 3; i < args.size(); ++i) {
    std::string upper(args[i]);
    utils::ToUppercase(upper);
    if (std::string_view(upper) == kFlagByScore) {
      if (has_by_score) {
        return false;
      }
      has_by_score = true;
    } else if (std::string_view(upper) == kFlagReverse) {
      if (has_reverse) {
        return false;
      }
      has_reverse = true;
    } else if (std::string_view(upper) == kFlagWithScores) {
      if (has_with_scores) {
        return false;
      }
      has_with_scores = true;
    } else if (std::string_view(upper) == kFlagLimit) {
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

int ParseRangeToRankSpec(const CommandArgs& args, RangeByRankSpec* const spec) {
  if (args.size() < 3 || !ValidateRangeOptions(args)) {
    return -1;
  }
  std::string_view start = args[1];
  std::string_view end = args[2];
  if (ParseRankRange(start, end, spec) < 0) {
    return -1;
  }
  spec->limit = std::make_unique<LimitSpec>();
  if (ParseLimitOffsetAndCount(args, spec->limit) < 0) {
    return -1;
  }
  spec->reverse = IsReverse(args);
  return 0;
}

int ParseRankRange(std::string_view start, std::string_view end,
                   RangeByRankSpec* const spec) {
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

int ParseRangeTerm(std::string_view term, long* const dst) {
  if (term.empty() || (dst == nullptr)) {
    return -1;
  }
  if (std::string_view(term) == kMinVal) {
    *dst = 0;
  } else if (std::string_view(term) == kMaxVal) {
    *dst = std::numeric_limits<long>::max();
  } else if (term[0] == '(') {
    try {
      *dst = std::stol(std::string(term.substr(1)));
    } catch (const std::exception&) {
      return -1;
    }
  } else {
    try {
      *dst = std::stol(std::string(term));
    } catch (const std::exception&) {
      return -1;
    }
  }
  return 0;
}

int ParseRangeToScoreSpec(const CommandArgs& args,
                          RangeByScoreSpec* const spec) {
  if (args.size() < 3 || !ValidateRangeOptions(args)) {
    return -1;
  }
  std::string_view start = args[1];
  std::string_view end = args[2];
  if (ParseScoreRange(start, end, spec) < 0) {
    return -1;
  }
  spec->limit = std::make_unique<LimitSpec>();
  if (ParseLimitOffsetAndCount(args, spec->limit) < 0) {
    return -1;
  }
  spec->reverse = IsReverse(args);
  return 0;
}

int ParseScoreRange(std::string_view start, std::string_view end,
                    RangeByScoreSpec* const spec) {
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

int ParseScoreTerm(std::string_view term, double* const dst) {
  if (term.empty() || (dst == nullptr)) {
    return -1;
  }
  if (std::string_view(term) == kMinVal) {
    *dst = -std::numeric_limits<double>::infinity();
  } else if (std::string_view(term) == kMaxVal) {
    *dst = std::numeric_limits<double>::infinity();
  } else if (term[0] == '(') {
    try {
      *dst = std::stod(std::string(term.substr(1)));
    } catch (const std::exception&) {
      return -1;
    }
  } else {
    try {
      *dst = std::stod(std::string(term));
    } catch (const std::exception&) {
      return -1;
    }
  }
  return 0;
}

int ParseLimitOffsetAndCount(const CommandArgs& args,
                             const std::unique_ptr<LimitSpec>& spec) {
  // LIMIT is optional; when absent, the zset implementation uses an unbounded
  // range.
  size_t i = 3;
  for (; i < args.size(); ++i) {
    std::string upper(args[i]);
    utils::ToUppercase(upper);
    if (std::string_view(upper) == kFlagLimit) {
      break;
    }
  }
  if (i == args.size()) {
    spec->offset = 0;
    spec->count = std::nullopt;
    return 0;
  }
  if (i + 2 >= args.size()) {
    return -1;
  }
  long offset = 0;
  long count = 0;
  try {
    offset = std::stol(std::string(args[i + 1]));
  } catch (const std::exception&) {
    return -1;
  }
  try {
    count = std::stol(std::string(args[i + 2]));
  } catch (const std::exception&) {
    return -1;
  }
  if (offset < 0) {
    return -1;
  }
  spec->offset = static_cast<size_t>(offset);
  spec->count = count < 0 ? std::nullopt
                          : std::optional<size_t>(static_cast<size_t>(count));
  return 0;
}

bool IsReverse(const CommandArgs& args) {
  // REV is an option token, so it can appear after key/start/end.
  for (size_t i = 3; i < args.size(); ++i) {
    std::string upper(args[i]);
    utils::ToUppercase(upper);
    if (std::string_view(upper) == kFlagReverse) {
      return true;
    }
  }
  return false;
}

bool IsWithScores(const CommandArgs& args) {
  for (size_t i = 3; i < args.size(); ++i) {
    std::string upper(args[i]);
    utils::ToUppercase(upper);
    if (std::string_view(upper) == kFlagWithScores) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> EncodeZRangeReply(const ZSetEntryList& result,
                                             bool with_scores) {
  const size_t reply_size = with_scores ? result.size() * 2 : result.size();
  std::string encoded = reply::FromArrayHeader(reply_size);
  for (const auto* entry : result) {
    reply::AppendBulkString(entry->key, &encoded);
    if (with_scores) {
      encoded.append(reply::FromFloat(entry->score));
    }
  }
  return encoded;
}

std::optional<int64_t> ToReplyInteger(size_t value) {
  if (value > static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
    return std::nullopt;
  }
  return static_cast<int64_t>(value);
}
}  // namespace

void HandleZRange(Client* const client) {
  const auto& args = client->Args();
  AddRangeReply(client, args, FlaggedByScore(args));
}

void HandleZRevRange(Client* const client) {
  auto args = client->Args();
  args.emplace_back(kFlagReverse);
  AddRangeReply(client, args, false);
}

void HandleZRangeByScore(Client* const client) {
  auto args = client->Args();
  args.emplace_back(kFlagByScore);
  AddRangeReply(client, args, true);
}

void HandleZCount(Client* const client) {
  const auto& args = client->Args();
  if (args.size() != 3) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  RangeByScoreSpec spec;
  if (ParseScoreRange(args[1], args[2], &spec) < 0) {
    client->AddReply(reply::SyntaxError());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto* obj = redis_db->LookupKey(args[0]);
    if (obj == nullptr) {
      client->AddReply(reply::FromInt64(0));
      return;
    }
    if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    const auto count = ToReplyInteger(obj->ZSet()->Count(&spec));
    client->AddReply(count.has_value()
                         ? reply::FromInt64(*count)
                         : reply::FromError("ERR zset count out of range"));
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

namespace {

void AddRangeReply(Client* const client, const CommandArgs& args,
                   bool by_score) {
  ZSetEntryList range_entries;
  const int status = by_score ? RangeByScore(client, args, &range_entries)
                              : RangeByRank(client, args, &range_entries);
  if (status < 0) {
    if (status == kRangeWrongType) {
      client->AddReply(reply::WrongTypeError());
    } else if (status == kRangeDbUnavailable) {
      client->AddReply(reply::FromError("ERR db unavailable"));
    } else {
      client->AddReply(reply::SyntaxError());
    }
    return;
  }
  const auto reply = EncodeZRangeReply(range_entries, IsWithScores(args));
  if (reply.has_value()) {
    client->AddReply(*reply);
  } else {
    client->AddReply(reply::FromError("ERR zrange encode failed"));
  }
}

int RangeByRank(Client* const client, const CommandArgs& args,
                ZSetEntryList* result) {
  RangeByRankSpec spec;
  if (ParseRangeToRankSpec(args, &spec) < 0) {
    RS_LOG_DEBUG("invalid arguments for zrange rank\n");
    return kRangeSyntaxError;
  }
  if (auto* redis_db = client->Db()) {
    const auto& key = args[0];
    const auto* obj = redis_db->LookupKey(key);
    if (obj == nullptr) {
      return 0;
    }
    if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
      RS_LOG_DEBUG("incorrect value type\n");
      return kRangeWrongType;
    }
    try {
      auto* const zset = obj->ZSet();
      *result = zset->RangeByRank(&spec);
    } catch (const std::exception& e) {
      RS_LOG_DEBUG("catch exception %s", e.what());
      return kRangeSyntaxError;
    }
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    return kRangeDbUnavailable;
  }
  return 0;
}

int RangeByScore(Client* const client, const CommandArgs& args,
                 ZSetEntryList* result) {
  RangeByScoreSpec spec;
  if (ParseRangeToScoreSpec(args, &spec) < 0) {
    RS_LOG_DEBUG("invalid arguments for zrange score\n");
    return kRangeSyntaxError;
  }
  if (auto* redis_db = client->Db()) {
    const auto& key = args[0];
    const auto* obj = redis_db->LookupKey(key);
    if (obj == nullptr) {
      return 0;
    }
    if (obj->Type() != db::RedisObject::ObjectType::kZSet) {
      RS_LOG_DEBUG("incorrect value type\n");
      return kRangeWrongType;
    }
    try {
      const auto* zset = obj->ZSet();
      *result = zset->RangeByScore(&spec);
    } catch (const std::exception& e) {
      RS_LOG_DEBUG("catch exception %s", e.what());
      return kRangeSyntaxError;
    }
  } else {
    RS_LOG_DEBUG("db unavailable\n");
    return kRangeDbUnavailable;
  }
  return 0;
}
}  // namespace
}  // namespace redis_simple::command::zsets
