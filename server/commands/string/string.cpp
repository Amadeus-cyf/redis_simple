#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"
#include "utils/string_utils.h"

namespace redis_simple::command::strings {
namespace {
enum class StringStatus : std::uint8_t {
  kOk,
  kMissing,
  kWrongType,
  kError,
};

struct StringResult {
  const std::string* value;
  StringStatus status;
};

StringResult LookupString(db::RedisDb* const redis_db, std::string_view key) {
  const auto* object = redis_db->LookupKey(key);
  if (object == nullptr) {
    return {nullptr, StringStatus::kMissing};
  }
  if (object->Type() != db::RedisObject::ObjectType::kString) {
    return {nullptr, StringStatus::kWrongType};
  }
  return {&object->String(), StringStatus::kOk};
}

std::optional<int64_t> ToReplyInteger(size_t value) {
  if (value > static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
    return std::nullopt;
  }
  return static_cast<int64_t>(value);
}

int64_t IncrementValue(int64_t value, int64_t increment, bool* ok) {
  if ((increment > 0 &&
       value > std::numeric_limits<int64_t>::max() - increment) ||
      (increment < 0 &&
       value < std::numeric_limits<int64_t>::min() - increment)) {
    *ok = false;
    return 0;
  }
  *ok = true;
  return value + increment;
}

void HandleIncrement(Client* const client, int64_t increment) {
  const auto& args = client->Args();
  if (args.size() != 1) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  auto* redis_db = client->Db();
  if (redis_db == nullptr) {
    client->AddReply(reply::FromError("ERR db unavailable"));
    return;
  }

  const auto current = LookupString(redis_db, args[0]);
  if (current.status == StringStatus::kWrongType) {
    client->AddReply(reply::WrongTypeError());
    return;
  }
  int64_t value = 0;
  if (current.status == StringStatus::kOk &&
      !utils::ToInt64(*current.value, &value)) {
    client->AddReply(reply::FromError("ERR value is not an integer"));
    return;
  }
  bool ok = false;
  const int64_t next = IncrementValue(value, increment, &ok);
  if (!ok) {
    client->AddReply(
        reply::FromError("ERR increment or decrement would overflow"));
    return;
  }
  auto object = db::RedisObject::CreateWithString(std::to_string(next));
  redis_db->SetKey(args[0], std::move(object), 0,
                   db::ToInt(db::SetKeyFlag::kKeepTtl));
  client->AddReply(reply::FromInt64(next));
}
}  // namespace

void HandleIncr(Client* const client) { HandleIncrement(client, 1); }

void HandleDecr(Client* const client) { HandleIncrement(client, -1); }

void HandleAppend(Client* const client) {
  const auto& args = client->Args();
  if (args.size() != 2) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  auto* redis_db = client->Db();
  if (redis_db == nullptr) {
    client->AddReply(reply::FromError("ERR db unavailable"));
    return;
  }
  std::string_view key = args[0];
  std::string_view value_to_append = args[1];
  auto* object = redis_db->MutableLookupKey(key);
  if (object != nullptr &&
      object->Type() != db::RedisObject::ObjectType::kString) {
    client->AddReply(reply::WrongTypeError());
    return;
  }
  if (object == nullptr) {
    const auto length = ToReplyInteger(value_to_append.size());
    if (!length.has_value()) {
      client->AddReply(reply::FromError("ERR string length out of range"));
      return;
    }
    redis_db->SetKey(
        key, db::RedisObject::CreateWithString(std::string(value_to_append)),
        0);
    client->AddReply(reply::FromInt64(*length));
    return;
  }
  std::string* const value = object->MutableString();
  value->append(value_to_append);
  const auto length = ToReplyInteger(value->size());
  client->AddReply(length.has_value()
                       ? reply::FromInt64(*length)
                       : reply::FromError("ERR string length out of range"));
}

void HandleMGet(Client* const client) {
  const auto& keys = client->Args();
  if (keys.empty()) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  auto* redis_db = client->Db();
  if (redis_db == nullptr) {
    client->AddReply(reply::FromError("ERR db unavailable"));
    return;
  }
  std::string encoded = reply::FromArrayHeader(keys.size());
  for (const auto& key : keys) {
    const auto result = LookupString(redis_db, key);
    if (result.status == StringStatus::kOk) {
      reply::AppendBulkString(*result.value, &encoded);
    } else {
      encoded.append(reply::Null());
    }
  }
  client->AddReply(encoded);
}

void HandleMSet(Client* const client) {
  const auto& args = client->Args();
  if (args.empty() || args.size() % 2 != 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }
  auto* redis_db = client->Db();
  if (redis_db == nullptr) {
    client->AddReply(reply::FromError("ERR db unavailable"));
    return;
  }
  for (size_t i = 0; i < args.size(); i += 2) {
    redis_db->SetKey(
        args[i], db::RedisObject::CreateWithString(std::string(args[i + 1])),
        0);
  }
  client->AddReply(reply::FromString("OK"));
}
}  // namespace redis_simple::command::strings
