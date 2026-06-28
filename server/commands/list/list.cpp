#include <sys/types.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "server/client.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "storage/list/list.h"
#include "utils/string_utils.h"

namespace redis_simple::command::lists {
namespace {
using List = ::redis_simple::list::List;

enum class PushSide : std::uint8_t {
  kLeft,
  kRight,
};

enum class PopSide : std::uint8_t {
  kLeft,
  kRight,
};

enum class ListStatus : std::uint8_t {
  kOk,
  kMissing,
  kWrongType,
  kError,
};

struct PushArgs {
  std::string key;
  std::vector<std::string> values;
};

struct KeyArgs {
  std::string key;
};

struct RangeArgs {
  std::string key;
  int64_t start{0};
  int64_t stop{0};
};

struct ListResult {
  List* list;
  ListStatus status;
};

struct PopResult {
  std::optional<std::string> value;
  ListStatus status;
};

int ParsePushArgs(const std::vector<std::string>& args, PushArgs* push_args);
int ParseKeyArgs(const std::vector<std::string>& args, KeyArgs* key_args);
int ParseRangeArgs(const std::vector<std::string>& args,
                   RangeArgs* range_args);
ListResult FindList(db::RedisDb* redis_db, const std::string& key);
ListResult GetOrCreateList(db::RedisDb* redis_db, const std::string& key);
std::optional<std::pair<size_t, size_t>> NormalizeRange(int64_t start,
                                                        int64_t stop,
                                                        size_t size);
ssize_t Push(db::RedisDb* redis_db, const PushArgs* args, PushSide side);
PopResult Pop(db::RedisDb* redis_db, const KeyArgs* args, PopSide side);
std::optional<ssize_t> LLen(db::RedisDb* redis_db, const KeyArgs* args);
std::optional<std::vector<std::string>> LRange(db::RedisDb* redis_db,
                                               const RangeArgs* args);

int ParsePushArgs(const std::vector<std::string>& args,
                  PushArgs* const push_args) {
  if (args.size() < 2) {
    return -1;
  }
  push_args->key = args[0];
  push_args->values.assign(args.begin() + 1, args.end());
  return 0;
}

int ParseKeyArgs(const std::vector<std::string>& args, KeyArgs* const key_args) {
  if (args.size() != 1) {
    return -1;
  }
  key_args->key = args[0];
  return 0;
}

int ParseRangeArgs(const std::vector<std::string>& args,
                   RangeArgs* const range_args) {
  if (args.size() != 3) {
    return -1;
  }
  range_args->key = args[0];
  return utils::ToInt64(args[1], &range_args->start) &&
                 utils::ToInt64(args[2], &range_args->stop)
             ? 0
             : -1;
}

ListResult FindList(db::RedisDb* const redis_db, const std::string& key) {
  const auto* obj = redis_db->LookupKey(key);
  if (obj != nullptr && obj->Type() != db::RedisObject::ObjectType::kList) {
    return {nullptr, ListStatus::kWrongType};
  }
  if (obj == nullptr) {
    return {nullptr, ListStatus::kMissing};
  }
  return {obj->List(), ListStatus::kOk};
}

ListResult GetOrCreateList(db::RedisDb* const redis_db,
                           const std::string& key) {
  ListResult result = FindList(redis_db, key);
  if (result.status != ListStatus::kMissing) {
    return result;
  }
  auto new_obj =
      db::RedisObject::CreateWithList(std::unique_ptr<List>(List::Init()));
  const auto* obj = new_obj.get();
  if (redis_db->SetKey(key, std::move(new_obj), 0) == db::DbStatus::kError) {
    return {nullptr, ListStatus::kError};
  }
  return {obj->List(), ListStatus::kOk};
}

std::optional<std::pair<size_t, size_t>> NormalizeRange(int64_t start,
                                                        int64_t stop,
                                                        size_t size) {
  if (size == 0) {
    return std::nullopt;
  }
  const auto list_size = static_cast<int64_t>(size);
  if (start < 0) {
    start += list_size;
  }
  if (stop < 0) {
    stop += list_size;
  }
  start = std::max<int64_t>(start, 0);
  stop = std::min<int64_t>(stop, list_size - 1);
  if (start > stop || start >= list_size) {
    return std::nullopt;
  }
  return std::pair<size_t, size_t>(static_cast<size_t>(start),
                                   static_cast<size_t>(stop));
}

ssize_t Push(db::RedisDb* const redis_db, const PushArgs* const args,
             PushSide side) {
  const ListResult result = GetOrCreateList(redis_db, args->key);
  if (result.status != ListStatus::kOk) {
    return -1;
  }
  for (const auto& value : args->values) {
    const bool pushed =
        side == PushSide::kLeft ? result.list->LPush(value)
                                : result.list->RPush(value);
    if (!pushed) {
      return -1;
    }
  }
  return static_cast<ssize_t>(result.list->Size());
}

PopResult Pop(db::RedisDb* const redis_db, const KeyArgs* const args,
              PopSide side) {
  const ListResult result = FindList(redis_db, args->key);
  if (result.status == ListStatus::kMissing) {
    return {std::nullopt, ListStatus::kMissing};
  }
  if (result.status != ListStatus::kOk) {
    return {std::nullopt, result.status};
  }
  auto value = side == PopSide::kLeft ? result.list->LPop()
                                      : result.list->RPop();
  if (result.list->Size() == 0) {
    redis_db->DeleteKey(args->key);
  }
  return {value, ListStatus::kOk};
}

std::optional<ssize_t> LLen(db::RedisDb* const redis_db,
                            const KeyArgs* const args) {
  const ListResult result = FindList(redis_db, args->key);
  if (result.status == ListStatus::kMissing) {
    return 0;
  }
  if (result.status != ListStatus::kOk) {
    return std::nullopt;
  }
  return static_cast<ssize_t>(result.list->Size());
}

std::optional<std::vector<std::string>> LRange(db::RedisDb* const redis_db,
                                               const RangeArgs* const args) {
  const ListResult result = FindList(redis_db, args->key);
  if (result.status == ListStatus::kMissing) {
    return std::vector<std::string>();
  }
  if (result.status != ListStatus::kOk) {
    return std::nullopt;
  }
  const auto range =
      NormalizeRange(args->start, args->stop, result.list->Size());
  if (!range.has_value()) {
    return std::vector<std::string>();
  }
  return result.list->Range(range->first, range->second);
}

void HandlePush(Client* const client, PushSide side) {
  PushArgs args;
  if (ParsePushArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const ssize_t result = Push(redis_db, &args, side);
    client->AddReply(result < 0 ? reply::FromInt64(reply::ReplyStatus::kError)
                                : reply::FromInt64(result));
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}

void HandlePop(Client* const client, PopSide side) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const PopResult result = Pop(redis_db, &args, side);
    if (result.status != ListStatus::kOk &&
        result.status != ListStatus::kMissing) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    } else if (result.value.has_value()) {
      client->AddReply(reply::FromBulkString(*result.value));
    } else {
      client->AddReply(reply::Null());
    }
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}
}  // namespace

void HandleLPush(Client* const client) { HandlePush(client, PushSide::kLeft); }

void HandleRPush(Client* const client) {
  HandlePush(client, PushSide::kRight);
}

void HandleLPop(Client* const client) { HandlePop(client, PopSide::kLeft); }

void HandleRPop(Client* const client) { HandlePop(client, PopSide::kRight); }

void HandleLLen(Client* const client) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto length = LLen(redis_db, &args);
    client->AddReply(length.has_value()
                         ? reply::FromInt64(*length)
                         : reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}

void HandleLRange(Client* const client) {
  RangeArgs args;
  if (ParseRangeArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto values = LRange(redis_db, &args);
    if (!values.has_value()) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    std::vector<std::string> encoded_values;
    encoded_values.reserve(values->size());
    for (const auto& value : *values) {
      encoded_values.push_back(reply::FromBulkString(value));
    }
    client->AddReply(reply::FromArray(encoded_values));
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}
}  // namespace redis_simple::command::lists
