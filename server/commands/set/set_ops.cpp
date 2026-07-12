#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
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
using SetOperation = std::optional<std::string> (*)(db::RedisDb*,
                                                    const KeysArgs*);

std::string BulkBodyToArrayReply(size_t count, std::string body);
std::optional<std::string> SInter(db::RedisDb* redis_db, const KeysArgs* args);
std::optional<std::string> SUnion(db::RedisDb* redis_db, const KeysArgs* args);
std::optional<std::string> SDiff(db::RedisDb* redis_db, const KeysArgs* args);
void AddSetOperationReply(Client* client, SetOperation operation);

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

std::string BulkBodyToArrayReply(size_t count, std::string body) {
  body.insert(0, reply::FromArrayHeader(count));
  return body;
}

std::optional<std::string> SInter(db::RedisDb* const redis_db,
                                  const KeysArgs* const args) {
  std::vector<const Set*> sets;
  sets.reserve(args->keys.size());
  for (const auto& key : args->keys) {
    const SetLookup lookup = FindSet(redis_db, key);
    if (lookup.status == SetLookupStatus::kMissing) {
      return reply::FromArrayHeader(0);
    }
    if (lookup.status != SetLookupStatus::kOk) {
      return std::nullopt;
    }
    sets.push_back(lookup.set);
  }

  auto smallest = std::min_element(sets.begin(), sets.end(),
                                   [](const Set* left, const Set* right) {
                                     return left->Size() < right->Size();
                                   });
  std::string body;
  size_t count = 0;
  (*smallest)->ForEachMember(
      [&sets, &body, &count, smallest](std::string_view member) {
        bool present = true;
        for (const auto* set : sets) {
          if (set != *smallest && !set->HasMember(member)) {
            present = false;
            break;
          }
        }
        if (present) {
          reply::AppendBulkString(member, &body);
          ++count;
        }
        return true;
      });
  return BulkBodyToArrayReply(count, std::move(body));
}

std::optional<std::string> SUnion(db::RedisDb* const redis_db,
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
    members.reserve(members.size() + lookup.set->Size());
    lookup.set->ForEachMember([&members](std::string_view member) {
      members.emplace(member.data(), member.size());
      return true;
    });
  }
  std::string body;
  for (const auto& member : members) {
    reply::AppendBulkString(member, &body);
  }
  return BulkBodyToArrayReply(members.size(), std::move(body));
}

std::optional<std::string> SDiff(db::RedisDb* const redis_db,
                                 const KeysArgs* const args) {
  const SetLookup first = FindSet(redis_db, args->keys.front());
  if (first.status == SetLookupStatus::kMissing) {
    return reply::FromArrayHeader(0);
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

  std::string body;
  size_t count = 0;
  first.set->ForEachMember(
      [&subtract_sets, &body, &count](std::string_view member) {
        bool removed = false;
        for (const auto* set : subtract_sets) {
          if (set->HasMember(member)) {
            removed = true;
            break;
          }
        }
        if (!removed) {
          reply::AppendBulkString(member, &body);
          ++count;
        }
        return true;
      });
  return BulkBodyToArrayReply(count, std::move(body));
}

void AddSetOperationReply(Client* const client, SetOperation operation) {
  KeysArgs args;
  if (ParseKeysArgs(client->Args(), &args) < 0) {
    client->AddReply(reply::WrongNumberOfArguments());
    return;
  }

  if (auto* redis_db = client->Db()) {
    const auto encoded = operation(redis_db, &args);
    client->AddReply(encoded.has_value() ? *encoded : reply::WrongTypeError());
    return;
  }
  client->AddReply(reply::FromError("ERR db unavailable"));
}
}  // namespace

void HandleSInter(Client* const client) {
  AddSetOperationReply(client, SInter);
}

void HandleSUnion(Client* const client) {
  AddSetOperationReply(client, SUnion);
}

void HandleSDiff(Client* const client) { AddSetOperationReply(client, SDiff); }
}  // namespace redis_simple::command::sets
