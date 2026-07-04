#include "data_types/hash/hash.h"

#include <sys/types.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::hashes {
namespace {
using Hash = ::redis_simple::hash::Hash;
using HashEntry = ::redis_simple::hash::Hash::Entry;

enum class HashStatus : std::uint8_t {
  kOk,
  kMissing,
  kWrongType,
  kError,
};

struct KeyArgs {
  std::string key;
};

struct FieldArgs {
  std::string key;
  std::string field;
};

struct SetArgs {
  std::string key;
  std::vector<HashEntry> entries;
};

struct DeleteArgs {
  std::string key;
  std::vector<std::string> fields;
};

struct HashResult {
  Hash* hash;
  HashStatus status;
};

struct GetResult {
  std::optional<std::string> value;
  HashStatus status;
};

int ParseKeyArgs(const std::vector<std::string>& args, KeyArgs* key_args);
int ParseFieldArgs(const std::vector<std::string>& args,
                   FieldArgs* field_args);
int ParseSetArgs(const std::vector<std::string>& args, SetArgs* set_args);
int ParseDeleteArgs(const std::vector<std::string>& args,
                    DeleteArgs* delete_args);
HashResult FindHash(db::RedisDb* redis_db, const std::string& key);
HashResult FindOrCreateHash(db::RedisDb* redis_db, const std::string& key);
std::optional<ssize_t> HSet(db::RedisDb* redis_db, const SetArgs* args);
GetResult HGet(db::RedisDb* redis_db, const FieldArgs* args);
std::optional<ssize_t> HDel(db::RedisDb* redis_db, const DeleteArgs* args);
std::optional<ssize_t> HLen(db::RedisDb* redis_db, const KeyArgs* args);
std::optional<ssize_t> HExists(db::RedisDb* redis_db, const FieldArgs* args);
std::optional<std::vector<HashEntry>> HGetAll(db::RedisDb* redis_db,
                                              const KeyArgs* args);
std::vector<std::string> EncodeEntries(const std::vector<HashEntry>& entries);

int ParseKeyArgs(const std::vector<std::string>& args,
                 KeyArgs* const key_args) {
  if (args.size() != 1) {
    return -1;
  }
  key_args->key = args[0];
  return 0;
}

int ParseFieldArgs(const std::vector<std::string>& args,
                   FieldArgs* const field_args) {
  if (args.size() != 2) {
    return -1;
  }
  field_args->key = args[0];
  field_args->field = args[1];
  return 0;
}

int ParseSetArgs(const std::vector<std::string>& args,
                 SetArgs* const set_args) {
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

int ParseDeleteArgs(const std::vector<std::string>& args,
                    DeleteArgs* const delete_args) {
  if (args.size() < 2) {
    return -1;
  }
  delete_args->key = args[0];
  delete_args->fields.assign(args.begin() + 1, args.end());
  return 0;
}

HashResult FindHash(db::RedisDb* const redis_db, const std::string& key) {
  const auto* obj = redis_db->LookupKey(key);
  if (obj != nullptr && obj->Type() != db::RedisObject::ObjectType::kHash) {
    return {nullptr, HashStatus::kWrongType};
  }
  if (obj == nullptr) {
    return {nullptr, HashStatus::kMissing};
  }
  return {obj->Hash(), HashStatus::kOk};
}

HashResult FindOrCreateHash(db::RedisDb* const redis_db,
                            const std::string& key) {
  HashResult result = FindHash(redis_db, key);
  if (result.status != HashStatus::kMissing) {
    return result;
  }
  auto new_obj = db::RedisObject::CreateWithHash(Hash::Create());
  const auto* obj = new_obj.get();
  if (redis_db->SetKey(key, std::move(new_obj), 0) == db::DbStatus::kError) {
    return {nullptr, HashStatus::kError};
  }
  return {obj->Hash(), HashStatus::kOk};
}

std::optional<ssize_t> HSet(db::RedisDb* const redis_db,
                            const SetArgs* const args) {
  const HashResult result = FindOrCreateHash(redis_db, args->key);
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  ssize_t added = 0;
  for (const auto& entry : args->entries) {
    added += result.hash->Set(entry.field, entry.value) ? 1 : 0;
  }
  return added;
}

GetResult HGet(db::RedisDb* const redis_db, const FieldArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return {std::nullopt, HashStatus::kMissing};
  }
  if (result.status != HashStatus::kOk) {
    return {std::nullopt, result.status};
  }
  return {result.hash->Get(args->field), HashStatus::kOk};
}

std::optional<ssize_t> HDel(db::RedisDb* const redis_db,
                            const DeleteArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return 0;
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  ssize_t deleted = 0;
  for (const auto& field : args->fields) {
    deleted += result.hash->Delete(field) ? 1 : 0;
  }
  if (result.hash->Size() == 0) {
    redis_db->DeleteKey(args->key);
  }
  return deleted;
}

std::optional<ssize_t> HLen(db::RedisDb* const redis_db,
                            const KeyArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return 0;
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  return static_cast<ssize_t>(result.hash->Size());
}

std::optional<ssize_t> HExists(db::RedisDb* const redis_db,
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

std::optional<std::vector<HashEntry>> HGetAll(db::RedisDb* const redis_db,
                                              const KeyArgs* const args) {
  const HashResult result = FindHash(redis_db, args->key);
  if (result.status == HashStatus::kMissing) {
    return std::vector<HashEntry>();
  }
  if (result.status != HashStatus::kOk) {
    return std::nullopt;
  }
  return result.hash->Entries();
}

std::vector<std::string> EncodeEntries(const std::vector<HashEntry>& entries) {
  std::vector<std::string> encoded;
  encoded.reserve(entries.size() * 2);
  for (const auto& entry : entries) {
    encoded.push_back(reply::FromBulkString(entry.field));
    encoded.push_back(reply::FromBulkString(entry.value));
  }
  return encoded;
}
}  // namespace

void HandleHSet(Client* const client) {
  SetArgs args;
  if (ParseSetArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HSet(redis_db, &args);
    client->AddReply(result.has_value()
                         ? reply::FromInt64(*result)
                         : reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}

void HandleHGet(Client* const client) {
  FieldArgs args;
  if (ParseFieldArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const GetResult result = HGet(redis_db, &args);
    if (result.status != HashStatus::kOk &&
        result.status != HashStatus::kMissing) {
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

void HandleHDel(Client* const client) {
  DeleteArgs args;
  if (ParseDeleteArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HDel(redis_db, &args);
    client->AddReply(result.has_value()
                         ? reply::FromInt64(*result)
                         : reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}

void HandleHLen(Client* const client) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HLen(redis_db, &args);
    client->AddReply(result.has_value()
                         ? reply::FromInt64(*result)
                         : reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}

void HandleHExists(Client* const client) {
  FieldArgs args;
  if (ParseFieldArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HExists(redis_db, &args);
    client->AddReply(result.has_value()
                         ? reply::FromInt64(*result)
                         : reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}

void HandleHGetAll(Client* const client) {
  KeyArgs args;
  if (ParseKeyArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto result = HGetAll(redis_db, &args);
    if (!result.has_value()) {
      client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
      return;
    }
    client->AddReply(reply::FromArray(EncodeEntries(*result)));
    return;
  }
  client->AddReply(reply::FromInt64(reply::ReplyStatus::kError));
}
}  // namespace redis_simple::command::hashes
