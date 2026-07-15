#include "data_types/hash/hash.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
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

namespace redis_simple::command::hashes {
namespace {
using Hash = ::redis_simple::hash::Hash;

struct HashEntryView {
  std::string_view field;
  std::string_view value;
};

enum class HashStatus : std::uint8_t {
  kOk,
  kMissing,
  kWrongType,
  kError,
};

struct KeyArgs {
  std::string_view key;
};

struct FieldArgs {
  std::string_view key;
  std::string_view field;
};

struct SetArgs {
  std::string_view key;
  std::vector<HashEntryView> entries;
};

struct DeleteArgs {
  std::string_view key;
  std::vector<std::string_view> fields;
};

struct FieldsArgs {
  std::string_view key;
  std::vector<std::string_view> fields;
};

struct IncrementArgs {
  std::string_view key;
  std::string_view field;
  int64_t increment{0};
};

struct HashResult {
  const Hash* hash{nullptr};
  HashStatus status{HashStatus::kMissing};
};

struct MutableHashResult {
  Hash* hash{nullptr};
  HashStatus status{HashStatus::kMissing};
};

struct IncrementResult {
  std::optional<int64_t> value;
  HashStatus status{HashStatus::kMissing};
};

int ParseKeyArgs(const CommandArgs& args, KeyArgs* key_args);
int ParseFieldArgs(const CommandArgs& args, FieldArgs* field_args);
int ParseSetArgs(const CommandArgs& args, SetArgs* set_args);
int ParseDeleteArgs(const CommandArgs& args, DeleteArgs* delete_args);
int ParseFieldsArgs(const CommandArgs& args, FieldsArgs* fields_args);
int ParseIncrementArgs(const CommandArgs& args, IncrementArgs* increment_args);
HashResult FindHash(db::RedisDb* redis_db, std::string_view key);
MutableHashResult FindMutableHash(db::RedisDb* redis_db, std::string_view key);
MutableHashResult FindOrCreateHash(db::RedisDb* redis_db, std::string_view key);
std::optional<int64_t> ToReplyInteger(size_t value);
bool AddWithOverflowCheck(int64_t value, int64_t increment, int64_t* result);
std::optional<int64_t> HSet(db::RedisDb* redis_db, const SetArgs* args);
std::optional<std::string> HGet(db::RedisDb* redis_db, const FieldArgs* args);
std::optional<int64_t> HDel(db::RedisDb* redis_db, const DeleteArgs* args);
std::optional<int64_t> HLen(db::RedisDb* redis_db, const KeyArgs* args);
std::optional<int64_t> HExists(db::RedisDb* redis_db, const FieldArgs* args);
std::optional<std::string> HGetAll(db::RedisDb* redis_db, const KeyArgs* args);
std::optional<std::string> HMGet(db::RedisDb* redis_db, const FieldsArgs* args);
std::optional<std::string> HKeys(db::RedisDb* redis_db, const KeyArgs* args);
std::optional<std::string> HVals(db::RedisDb* redis_db, const KeyArgs* args);
IncrementResult HIncrBy(db::RedisDb* redis_db, const IncrementArgs* args);

int ParseKeyArgs(const CommandArgs& args, KeyArgs* const key_args) {
  if (args.size() != 1) {
    return -1;
  }
  key_args->key = args[0];
  return 0;
}

int ParseFieldArgs(const CommandArgs& args, FieldArgs* const field_args) {
  if (args.size() != 2) {
    return -1;
  }
  field_args->key = args[0];
  field_args->field = args[1];
  return 0;
}

int ParseSetArgs(const CommandArgs& args, SetArgs* const set_args) {
  if (args.size() < 3 || args.size() % 2 == 0) {
    return -1;
  }
  set_args->key = args[0];
  set_args->entries.reserve((args.size() - 1) / 2);
  for (size_t i = 1; i < args.size(); i += 2) {
    set_args->entries.push_back({args[i], args[i + 1]});
  }
  return 0;
}

int ParseDeleteArgs(const CommandArgs& args, DeleteArgs* const delete_args) {
  if (args.size() < 2) {
    return -1;
  }
  delete_args->key = args[0];
  delete_args->fields.assign(args.begin() + 1, args.end());
  return 0;
}

int ParseFieldsArgs(const CommandArgs& args, FieldsArgs* const fields_args) {
  if (args.size() < 2) {
    return -1;
  }
  fields_args->key = args[0];
  fields_args->fields.assign(args.begin() + 1, args.end());
  return 0;
}

int ParseIncrementArgs(const CommandArgs& args,
                       IncrementArgs* const increment_args) {
  if (args.size() != 3 ||
      !utils::ToInt64(args[2], &increment_args->increment)) {
    return -1;
  }
  increment_args->key = args[0];
  increment_args->field = args[1];
  return 0;
}

HashResult FindHash(db::RedisDb* const redis_db, std::string_view key) {
  const auto* obj = redis_db->LookupKey(key);
  if (obj != nullptr && obj->Type() != db::RedisObject::ObjectType::kHash) {
    return {nullptr, HashStatus::kWrongType};
  }
  if (obj == nullptr) {
    return {nullptr, HashStatus::kMissing};
  }
  return {obj->Hash(), HashStatus::kOk};
}

MutableHashResult FindMutableHash(db::RedisDb* const redis_db,
                                  std::string_view key) {
  auto* obj = redis_db->MutableLookupKey(key);
  if (obj != nullptr && obj->Type() != db::RedisObject::ObjectType::kHash) {
    return {nullptr, HashStatus::kWrongType};
  }
  if (obj == nullptr) {
    return {nullptr, HashStatus::kMissing};
  }
  return {obj->Hash(), HashStatus::kOk};
}

MutableHashResult FindOrCreateHash(db::RedisDb* const redis_db,
                                   std::string_view key) {
  MutableHashResult result = FindMutableHash(redis_db, key);
  if (result.status != HashStatus::kMissing) {
    return result;
  }
  // LLVM 18 cannot trace unique_ptr ownership through Dict::Set.
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  auto new_obj = db::RedisObject::CreateWithHash(Hash::Create());
  auto* obj = new_obj.get();
  if (redis_db->SetKey(key, std::move(new_obj), 0) == db::DbStatus::kError) {
    return {nullptr, HashStatus::kError};
  }
  return {obj->Hash(), HashStatus::kOk};
}

std::optional<int64_t> ToReplyInteger(size_t value) {
  if (value > static_cast<size_t>(std::numeric_limits<int64_t>::max())) {
    return std::nullopt;
  }
  return static_cast<int64_t>(value);
}

bool AddWithOverflowCheck(int64_t value, int64_t increment, int64_t* result) {
  if ((increment > 0 &&
       value > std::numeric_limits<int64_t>::max() - increment) ||
      (increment < 0 &&
       value < std::numeric_limits<int64_t>::min() - increment)) {
    return false;
  }
  *result = value + increment;
  return true;
}

std::optional<int64_t> HSet(db::RedisDb* const redis_db,
                            const SetArgs* const args) {
  const MutableHashResult result = FindOrCreateHash(redis_db, args->key);
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  int64_t added = 0;
  for (const auto& entry : args->entries) {
    added += result.hash->Set(entry.field, entry.value) ? 1 : 0;
  }
  return added;
}

std::optional<std::string> HGet(db::RedisDb* const redis_db,
                                const FieldArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return reply::Null();
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  std::string encoded;
  if (!result.hash->VisitValue(args->field, [&encoded](std::string_view value) {
        encoded = reply::FromBulkString(value);
      })) {
    return reply::Null();
  }
  return encoded;
}

std::optional<int64_t> HDel(db::RedisDb* const redis_db,
                            const DeleteArgs* const args) {
  const MutableHashResult result = FindMutableHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return 0;
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  int64_t deleted = 0;
  for (const auto& field : args->fields) {
    deleted += result.hash->Delete(field) ? 1 : 0;
  }
  if (result.hash->Size() == 0) {
    redis_db->DeleteKey(args->key);
  }
  return deleted;
}

std::optional<int64_t> HLen(db::RedisDb* const redis_db,
                            const KeyArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return 0;
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  return ToReplyInteger(result.hash->Size());
}

std::optional<int64_t> HExists(db::RedisDb* const redis_db,
                               const FieldArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return 0;
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  return result.hash->Exists(args->field) ? 1 : 0;
}

std::optional<std::string> HGetAll(db::RedisDb* const redis_db,
                                   const KeyArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return reply::FromArrayHeader(0);
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  std::string encoded = reply::FromArrayHeader(result.hash->Size() * 2);
  result.hash->ForEachEntry(
      [&encoded](std::string_view field, std::string_view value) {
        reply::AppendBulkString(field, &encoded);
        reply::AppendBulkString(value, &encoded);
        return true;
      });
  return encoded;
}

std::optional<std::string> HMGet(db::RedisDb* const redis_db,
                                 const FieldsArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  std::string encoded = reply::FromArrayHeader(args->fields.size());
  if (result.status == HashStatus::kMissing) {
    for (size_t i = 0; i < args->fields.size(); ++i) {
      encoded.append(reply::Null());
    }
    return encoded;
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  for (const auto& field : args->fields) {
    if (!result.hash->VisitValue(field, [&encoded](std::string_view value) {
          reply::AppendBulkString(value, &encoded);
        })) {
      encoded.append(reply::Null());
    }
  }
  return encoded;
}

std::optional<std::string> HKeys(db::RedisDb* const redis_db,
                                 const KeyArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return reply::FromArrayHeader(0);
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  std::string encoded = reply::FromArrayHeader(result.hash->Size());
  result.hash->ForEachField([&encoded](std::string_view field) {
    reply::AppendBulkString(field, &encoded);
    return true;
  });
  return encoded;
}

std::optional<std::string> HVals(db::RedisDb* const redis_db,
                                 const KeyArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return reply::FromArrayHeader(0);
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  std::string encoded = reply::FromArrayHeader(result.hash->Size());
  result.hash->ForEachValue([&encoded](std::string_view value) {
    reply::AppendBulkString(value, &encoded);
    return true;
  });
  return encoded;
}

IncrementResult HIncrBy(db::RedisDb* const redis_db,
                        const IncrementArgs* const args) {
  const MutableHashResult result = FindOrCreateHash(redis_db, args->key);
  if (result.status != HashStatus::kOk) {
    return {std::nullopt, result.status};
  }
  int64_t current = 0;
  bool parsed = true;
  result.hash->VisitValue(args->field,
                          [&current, &parsed](std::string_view value) {
                            parsed = utils::ToInt64(value, &current);
                          });
  if (!parsed) {
    return {std::nullopt, HashStatus::kError};
  }
  int64_t next = 0;
  if (!AddWithOverflowCheck(current, args->increment, &next)) {
    return {std::nullopt, HashStatus::kError};
  }
  result.hash->Set(args->field, std::to_string(next));
  return {next, HashStatus::kOk};
}

}  // namespace

void HandleHSet(Client* const client) {
  SetArgs args;
  if (ParseSetArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HSet(redis_db, &args);
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHGet(Client* const client) {
  FieldArgs args;
  if (ParseFieldArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HGet(redis_db, &args);
    client->AddReply(result.has_value() ? *result : reply::WrongTypeError());
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHDel(Client* const client) {
  DeleteArgs args;
  if (ParseDeleteArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HDel(redis_db, &args);
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHLen(Client* const client) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HLen(redis_db, &args);
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHExists(Client* const client) {
  FieldArgs args;
  if (ParseFieldArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HExists(redis_db, &args);
    client->AddReply(result.has_value() ? reply::FromInt64(*result)
                                        : reply::WrongTypeError());
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHGetAll(Client* const client) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HGetAll(redis_db, &args);
    if (!result.has_value()) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    client->AddReply(*result);
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHMGet(Client* const client) {
  FieldsArgs args;
  if (ParseFieldsArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HMGet(redis_db, &args);
    if (!result.has_value()) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    client->AddReply(*result);
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHKeys(Client* const client) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HKeys(redis_db, &args);
    if (!result.has_value()) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    client->AddReply(*result);
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHVals(Client* const client) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HVals(redis_db, &args);
    if (!result.has_value()) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    client->AddReply(*result);
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}

void HandleHIncrBy(Client* const client) {
  IncrementArgs args;
  if (ParseIncrementArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const IncrementResult result = HIncrBy(redis_db, &args);
    if (result.status == HashStatus::kWrongType) {
      client->AddReply(reply::WrongTypeError());
      return;
    }
    if (!result.value.has_value()) {
      client->AddReply(
          reply::FromError("ERR hash value is not an integer or out of range"));
      return;
    }
    client->AddReply(reply::FromInt64(*result.value));
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}
}  // namespace redis_simple::command::hashes
