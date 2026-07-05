#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "data_types/set/set.h"
#include "server/client.h"
#include "server/commands/handlers.h"
#include "server/db/db.h"
#include "server/reply/reply.h"

namespace redis_simple::command::sets {
namespace {
using Set = ::redis_simple::set::Set;

enum class SetLookupStatus : std::uint8_t {
  kOk,
  kMissing,
  kWrongType,
};

struct KeysArgs {
  std::vector<std::string> keys;
};

struct SetLookup {
  const Set* set;
  SetLookupStatus status;
};

int ParseKeysArgs(const std::vector<std::string>& args, KeysArgs* keys_args);
SetLookup FindSet(db::RedisDb* redis_db, const std::string& key);
std::optional<std::vector<std::string>> SInter(db::RedisDb* redis_db,
                                               const KeysArgs* args);
std::optional<std::vector<std::string>> SUnion(db::RedisDb* redis_db,
                                               const KeysArgs* args);
std::optional<std::vector<std::string>> SDiff(db::RedisDb* redis_db,
                                              const KeysArgs* args);
std::string EncodeStrings(const std::vector<std::string>& values);
void AddSetOperationReply(
    Client* client,
    std::optional<std::vector<std::string>> (*operation)(db::RedisDb*,
                                                         const KeysArgs*));

int ParseKeysArgs(const std::vector<std::string>& args,
                  KeysArgs* const keys_args) {
  if (args.empty()) {
    return -1;
  }
  keys_args->keys = args;
  return 0;
}

SetLookup FindSet(db::RedisDb* const redis_db, const std::string& key) {
  const auto* object = redis_db->LookupKey(key);
  if (object == nullptr) {
    return {nullptr, SetLookupStatus::kMissing};
  }
  if (object->Type() != db::RedisObject::ObjectType::kSet) {
    return {nullptr, SetLookupStatus::kWrongType};
  }
  return {object->Set(), SetLookupStatus::kOk};
}

std::optional<std::vector<std::string>> SInter(db::RedisDb* const redis_db,
                                               const KeysArgs* const args) {
  std::vector<const Set*> sets;
  sets.reserve(args->keys.size());
  for (const auto& key : args->keys) {
    const SetLookup lookup = FindSet(redis_db, key);
    if (lookup.status == SetLookupStatus::kMissing) {
      return std::vector<std::string>();
    }
    if (lookup.status != SetLookupStatus::kOk) {
      return std::nullopt;
    }
    sets.push_back(lookup.set);
  }

  auto smallest = std::min_element(
      sets.begin(), sets.end(),
      [](const Set* left, const Set* right) { return left->Size() < right->Size(); });
  std::vector<std::string> result;
  for (const auto& member : (*smallest)->ListAllMembers()) {
    bool present = true;
    for (const auto* set : sets) {
      if (set != *smallest && !set->HasMember(member)) {
        present = false;
        break;
      }
    }
    if (present) {
      result.push_back(member);
    }
  }
  return result;
}

std::optional<std::vector<std::string>> SUnion(db::RedisDb* const redis_db,
                                               const KeysArgs* const args) {
  std::unordered_set<std::string> members;
  for (const auto& key : args->keys) {
    const SetLookup lookup = FindSet(redis_db, key);
    if (lookup.status == SetLookupStatus::kMissing) {
      continue;
    }
    if (lookup.status != SetLookupStatus::kOk) {
      return std::nullopt;
    }
    for (const auto& member : lookup.set->ListAllMembers()) {
      members.insert(member);
    }
  }
  return std::vector<std::string>(members.begin(), members.end());
}

std::optional<std::vector<std::string>> SDiff(db::RedisDb* const redis_db,
                                              const KeysArgs* const args) {
  const SetLookup first = FindSet(redis_db, args->keys.front());
  if (first.status == SetLookupStatus::kMissing) {
    return std::vector<std::string>();
  }
  if (first.status != SetLookupStatus::kOk) {
    return std::nullopt;
  }

  std::vector<const Set*> subtract_sets;
  subtract_sets.reserve(args->keys.size() - 1);
  for (auto it = args->keys.begin() + 1; it != args->keys.end(); ++it) {
    const SetLookup lookup = FindSet(redis_db, *it);
    if (lookup.status == SetLookupStatus::kMissing) {
      continue;
    }
    if (lookup.status != SetLookupStatus::kOk) {
      return std::nullopt;
    }
    subtract_sets.push_back(lookup.set);
  }

  std::vector<std::string> result;
  for (const auto& member : first.set->ListAllMembers()) {
    bool removed = false;
    for (const auto* set : subtract_sets) {
      if (set->HasMember(member)) {
        removed = true;
        break;
      }
    }
    if (!removed) {
      result.push_back(member);
    }
  }
  return result;
}

std::string EncodeStrings(const std::vector<std::string>& values) {
  std::vector<std::string> encoded;
  encoded.reserve(values.size());
  for (const auto& value : values) {
    encoded.push_back(reply::FromBulkString(value));
  }
  return reply::FromArray(encoded);
}

void AddSetOperationReply(
    Client* const client,
    std::optional<std::vector<std::string>> (*operation)(db::RedisDb*,
                                                         const KeysArgs*)) {
  KeysArgs args;
  if (ParseKeysArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto values = operation(redis_db, &args);
    client->AddReply(values.has_value() ? EncodeStrings(*values)
                                        : reply::WrongTypeError());
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}
}  // namespace

void HandleSInter(Client* const client) { AddSetOperationReply(client, SInter); }

void HandleSUnion(Client* const client) { AddSetOperationReply(client, SUnion); }

void HandleSDiff(Client* const client) { AddSetOperationReply(client, SDiff); }
}  // namespace redis_simple::command::sets
