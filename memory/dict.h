#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace redis_simple::in_memory {
template <typename K, typename V>
class Dict {
 public:
  class Iterator;
  struct DictType;
  static std::unique_ptr<Dict<K, V>> Create();
  static std::unique_ptr<Dict<K, V>> Create(size_t capacity);
  static std::unique_ptr<Dict<K, V>> Create(const DictType& type);
  Dict(const Dict&) = delete;
  Dict& operator=(const Dict&) = delete;
  std::optional<V> Get(const K& key);
  V* FindValue(const K& key);
  V* FindValue(std::string_view key);
  V* FindValue(const char* key);
  void Set(const K& key, const V& val);
  void Set(const K& key, V&& val);
  void Set(K&& key, V&& val);
  bool Insert(const K& key, const V& val);
  bool Insert(K&& key, V&& val);
  bool Delete(const K& key);
  bool Delete(std::string_view key);
  bool Delete(const char* key) { return Delete(std::string_view(key)); }
  template <typename Visitor>
  std::optional<size_t> Scan(size_t cursor, Visitor&& visitor);
  size_t Size() const { return table_used_[0] + table_used_[1]; }
  void Clear();
  ~Dict();

 private:
  struct DictEntry;
  Dict();
  Dict(const DictType& type);
  void InitializeTables();
  void InitializeTableWithSize(int i, int exp, size_t size);
  void InsertEntry(DictEntry* entry, int i);
  DictEntry* Unlink(const K& key);
  DictEntry* Unlink(std::string_view key);
  void UnlinkEntry(DictEntry* entry, DictEntry* prev, int i);
  void DeleteEntry(DictEntry* entry, DictEntry* prev, int i);
  size_t TableSize(int exp) const {
    return exp < 0 || exp >= std::numeric_limits<size_t>::digits
               ? 0
               : size_t{1} << exp;
  }
  bool IsRehashing() const { return rehash_idx_.has_value(); }
  void PauseRehashing() { ++pause_rehash_; }
  void ResumeRehashing() {
    if (pause_rehash_ > 0) {
      --pause_rehash_;
    }
  }
  size_t TableMask(int i) const;
  size_t HashIndex(size_t hash, int i) const;
  size_t KeyHash(const K& key) const;
  size_t StringViewKeyHash(std::string_view key) const;
  int NextExp(size_t size) const;
  bool IsEqual(const K& key1, const K& key2) const;
  bool IsEqual(std::string_view key1, const K& key2) const;
  void SetKey(DictEntry* entry, const K& key);
  void SetKey(DictEntry* entry, K&& key);
  void SetVal(DictEntry* entry, const V& val);
  void SetVal(DictEntry* entry, V&& val);
  void FreeKey(DictEntry* entry);
  void FreeVal(DictEntry* entry);
  void FreeUnlinkedEntry(DictEntry* entry);
  DictEntry* FindEntry(const K& key);
  DictEntry* FindEntry(std::string_view key);
  std::optional<size_t> KeyIndex(const K& key, size_t hash,
                                 DictEntry** existing);
  DictEntry* InsertRaw(const K& key, DictEntry** existing);
  DictEntry* InsertRaw(K&& key, DictEntry** existing);
  void ExpandIfNeeded();
  bool Expand(size_t size);
  void RehashStepIfNeeded();
  void MigrateRehashedTable();
  bool Rehash(int n);
  void Clear(int i);
  void Reset(int i);
  static constexpr int kTableInitSize = 2;
  static constexpr int kTableInitExp = 1;
  // Rehash when elements/table-size reaches this ratio.
  static constexpr double kDictForceResizeRatio = 2.0;
  DictType type_;
  // Table 1 is populated incrementally while table 0 is being rehashed.
  std::array<std::vector<DictEntry*>, 2> tables_;
  std::array<size_t, 2> table_used_{};
  std::array<int, 2> table_size_exp_{};
  std::optional<size_t> rehash_idx_;
  size_t pause_rehash_;
};

template <typename K, typename V>
struct Dict<K, V>::DictEntry {
  K key;
  V val;
  size_t hash{};
  DictEntry* next{nullptr};
};

template <typename K, typename V>
struct Dict<K, V>::DictType {
  // Used to get the hash index of the key. Use std::hash by default.
  std::function<size_t(const K& key)> hash_function;
  // Optional non-owning lookup hooks for std::string-keyed dicts.
  std::function<size_t(std::string_view key)> string_view_hash_function;
  std::function<int(std::string_view key1, const K& key2)>
      string_view_key_compare;
  // If set, all keys will be copied while being inserted into the dict.
  std::function<K(const K& key)> key_dup;
  // If set, all values will be copied while being inserted into the dict.
  std::function<V(const V& val)> val_dup;
  // Callback function when the key is freed from the dict.
  std::function<void(K& key)> key_destructor;
  // Callback function when the value is freed from the dict.
  std::function<void(V& val)> val_destructor;
  // Comparator for keys. Used to check whether two keys are equal. Use operator
  // by default.
  std::function<int(const K& key1, const K& key2)> key_compare;
};

template <typename K, typename V>
class Dict<K, V>::Iterator {
 public:
  explicit Iterator(const Dict* dict);
  Iterator& operator=(const Iterator& it) = default;
  bool operator==(const Iterator& it) const;
  bool operator!=(const Iterator& it) const;
  bool Valid() const;
  void SeekToFirst();
  void SeekToLast();
  void Next();
  Iterator& operator++();
  const K& Key() const { return entry_->key; }
  const V& Value() const { return entry_->val; }

 private:
  void SeekToNextEntry();
  const Dict* dict_;
  int table_;
  std::optional<size_t> idx_;
  const DictEntry* entry_;
};

template <typename K, typename V>
Dict<K, V>::Iterator::Iterator(const Dict* dict)
    : dict_(dict), table_(0), idx_(std::nullopt), entry_(nullptr) {}

template <typename K, typename V>
bool Dict<K, V>::Iterator::operator==(const Iterator& it) const {
  return dict_ == it.dict_ && table_ == it.table_ && idx_ == it.idx_ &&
         entry_ == it.entry_;
}

template <typename K, typename V>
bool Dict<K, V>::Iterator::operator!=(const Iterator& it) const {
  return !((*this) == it);
}

template <typename K, typename V>
bool Dict<K, V>::Iterator::Valid() const {
  return entry_ != nullptr;
}

template <typename K, typename V>
void Dict<K, V>::Iterator::SeekToFirst() {
  table_ = 0;
  idx_ = std::nullopt;
  entry_ = nullptr;
  SeekToNextEntry();
}

template <typename K, typename V>
void Dict<K, V>::Iterator::SeekToLast() {
  entry_ = nullptr;
  const int last_table = dict_->IsRehashing() ? 1 : 0;
  for (table_ = last_table; table_ >= 0; --table_) {
    for (size_t bucket = dict_->tables_[table_].size(); bucket > 0; --bucket) {
      const size_t index = bucket - 1;
      if (dict_->tables_[table_][index] == nullptr) {
        continue;
      }
      idx_ = index;
      entry_ = dict_->tables_[table_][index];
      while (entry_->next != nullptr) {
        entry_ = entry_->next;
      }
      return;
    }
  }
  table_ = 0;
  idx_ = std::nullopt;
}

template <typename K, typename V>
typename Dict<K, V>::Iterator& Dict<K, V>::Iterator::operator++() {
  SeekToNextEntry();
  return *this;
}

template <typename K, typename V>
void Dict<K, V>::Iterator::Next() {
  SeekToNextEntry();
}

/*
 * Find the next non-null entry in the dict.
 */
template <typename K, typename V>
void Dict<K, V>::Iterator::SeekToNextEntry() {
  if (entry_ && entry_->next) {
    entry_ = entry_->next;
    return;
  }
  idx_ = idx_.has_value() ? std::optional<size_t>(*idx_ + 1) : size_t{0};
  // Find next non empty table entry list.
  while (*idx_ < dict_->tables_[table_].size() &&
         dict_->tables_[table_][*idx_] == nullptr) {
    ++*idx_;
  }
  if (*idx_ < dict_->tables_[table_].size()) {
    entry_ = dict_->tables_[table_][*idx_];
    return;
  }
  if (table_ == 1 || !dict_->IsRehashing()) {
    entry_ = nullptr;
    return;
  }
  // Find the next element in the rehashed table.
  idx_ = std::nullopt;
  table_ = 1;
  SeekToNextEntry();
}

/*
 * Initialize the dict with default functions.
 */
template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Create() {
  return Create(kTableInitSize);
}

/*
 * Initialize the dict with default functions and custom capacity.
 */
template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Create(size_t capacity) {
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>());
  if (!dict->Expand(capacity)) {
    return nullptr;
  }
  dict->type_.hash_function = [](const K& key) {
    if constexpr (std::is_same<K, std::string>::value) {
      std::hash<std::string_view> h;
      return h(std::string_view(key));
    } else {
      std::hash<K> h;
      return h(key);
    }
  };
  if constexpr (std::is_same<K, std::string>::value) {
    dict->type_.string_view_hash_function = [](std::string_view key) {
      std::hash<std::string_view> h;
      return h(key);
    };
  }
  return dict;
}

/*
 * Initialize the dict with customized functions.
 */
template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Create(
    const typename Dict<K, V>::DictType& type) {
  if (!type.hash_function) {
    return nullptr;
  }
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>(type));
  if (!dict->Expand(kTableInitSize)) {
    return nullptr;
  }
  return dict;
}

/*
 * Find the entry containing the given key.
 */
template <typename K, typename V>
std::optional<V> Dict<K, V>::Get(const K& key) {
  DictEntry* entry = FindEntry(key);
  if (entry == nullptr) {
    return std::nullopt;
  }
  return entry->val;
}

template <typename K, typename V>
V* Dict<K, V>::FindValue(const K& key) {
  DictEntry* entry = FindEntry(key);
  return entry == nullptr ? nullptr : &entry->val;
}

template <typename K, typename V>
V* Dict<K, V>::FindValue(std::string_view key) {
  DictEntry* entry = FindEntry(key);
  return entry == nullptr ? nullptr : &entry->val;
}

template <typename K, typename V>
V* Dict<K, V>::FindValue(const char* key) {
  return FindValue(std::string_view(key));
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::FindEntry(const K& key) {
  RehashStepIfNeeded();
  size_t hash = KeyHash(key);
  for (size_t i = 0; i < tables_.size(); ++i) {
    if (tables_[i].empty()) {
      if (!IsRehashing()) {
        break;
      }
      continue;
    }
    size_t idx = HashIndex(hash, i);
    DictEntry* entry = tables_[i][idx];
    while (entry) {
      if (entry->hash == hash && IsEqual(key, entry->key)) {
        return entry;
      }
      entry = entry->next;
    }
    if (!IsRehashing()) {
      break;
    }
  }
  return nullptr;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::FindEntry(std::string_view key) {
  static_assert(std::is_same<K, std::string>::value,
                "std::string_view lookup only supports std::string keys");
  RehashStepIfNeeded();
  size_t hash = StringViewKeyHash(key);
  for (size_t i = 0; i < tables_.size(); ++i) {
    if (tables_[i].empty()) {
      if (!IsRehashing()) {
        break;
      }
      continue;
    }
    size_t idx = HashIndex(hash, i);
    DictEntry* entry = tables_[i][idx];
    while (entry) {
      if (entry->hash == hash && IsEqual(key, entry->key)) {
        return entry;
      }
      entry = entry->next;
    }
    if (!IsRehashing()) {
      break;
    }
  }
  return nullptr;
}

/*
 * Set the key-value pair.
 * If the key exists, replace the corresponding value with the new value.
 * Otherwise, insert a new key-value pair into the dict.
 */
template <typename K, typename V>
void Dict<K, V>::Set(const K& key, const V& val) {
  DictEntry* existing = nullptr;
  DictEntry* entry = InsertRaw(key, &existing);
  if (entry) {
    SetVal(entry, val);
  } else if (existing) {
    DictEntry auxentry = *existing;
    SetVal(existing, val);
    FreeVal(&auxentry);
  }
}

template <typename K, typename V>
void Dict<K, V>::Set(const K& key, V&& val) {
  DictEntry* existing = nullptr;
  DictEntry* entry = InsertRaw(key, &existing);
  if (entry) {
    SetVal(entry, std::move(val));
  } else if (existing) {
    DictEntry auxentry;
    auxentry.val = std::move(existing->val);
    SetVal(existing, std::move(val));
    FreeVal(&auxentry);
  }
}

template <typename K, typename V>
void Dict<K, V>::Set(K&& key, V&& val) {
  DictEntry* existing = nullptr;
  DictEntry* entry = InsertRaw(std::move(key), &existing);
  if (entry != nullptr) {
    SetVal(entry, std::move(val));
  } else if (existing != nullptr) {
    DictEntry auxentry;
    auxentry.val = std::move(existing->val);
    SetVal(existing, std::move(val));
    FreeVal(&auxentry);
  }
}

/*
 * Insert a new key-value pair to the dict.
 * Return false if the key already exists in the dict.
 */
template <typename K, typename V>
bool Dict<K, V>::Insert(const K& key, const V& val) {
  DictEntry* entry = InsertRaw(key, nullptr);
  if (!entry) {
    return false;
  }
  SetVal(entry, val);
  return true;
}

template <typename K, typename V>
bool Dict<K, V>::Insert(K&& key, V&& val) {
  DictEntry* entry = InsertRaw(std::move(key), nullptr);
  if (!entry) {
    return false;
  }
  SetVal(entry, std::move(val));
  return true;
}

/*
 * Delete the key from the dict, and free the memory of the entry used to store
 * the key-value pair.
 */
template <typename K, typename V>
bool Dict<K, V>::Delete(const K& key) {
  DictEntry* de = Unlink(key);
  if (de) {
    FreeUnlinkedEntry(de);
    return true;
  }
  return false;
}

template <typename K, typename V>
bool Dict<K, V>::Delete(std::string_view key) {
  static_assert(std::is_same<K, std::string>::value,
                "std::string_view deletion only supports std::string keys");
  DictEntry* entry = Unlink(key);
  if (entry == nullptr) {
    return false;
  }
  FreeUnlinkedEntry(entry);
  return true;
}

template <typename K, typename V>
template <typename Visitor>
std::optional<size_t> Dict<K, V>::Scan(size_t cursor, Visitor&& visitor) {
  // Scanning visits the same bucket index in both tables; pause incremental
  // rehashing so entries do not move while this cursor position is processed.
  PauseRehashing();
  if (cursor < tables_[0].size()) {
    const DictEntry* de = tables_[0][cursor];
    while (de) {
      const DictEntry* next = de->next;
      visitor(de->key, de->val);
      de = next;
    }
  }
  // During rehashing, the same cursor position can have entries in both tables.
  if (IsRehashing() && cursor < tables_[1].size()) {
    const DictEntry* de = tables_[1][cursor];
    while (de) {
      const DictEntry* next = de->next;
      visitor(de->key, de->val);
      de = next;
    }
  }
  if (++cursor >= std::max(tables_[0].size(), tables_[1].size())) {
    ResumeRehashing();
    return std::nullopt;
  }
  ResumeRehashing();
  return cursor;
}

template <typename K, typename V>
void Dict<K, V>::Clear() {
  Clear(0);
  Clear(1);
  rehash_idx_ = std::nullopt;
  pause_rehash_ = 0;
}

template <typename K, typename V>
Dict<K, V>::~Dict() {
  Clear();
}

template <typename K, typename V>
Dict<K, V>::Dict() : rehash_idx_(std::nullopt), pause_rehash_(0) {
  InitializeTables();
}

template <typename K, typename V>
Dict<K, V>::Dict(const DictType& type)
    : type_(type), rehash_idx_(std::nullopt), pause_rehash_(0) {
  InitializeTables();
}

template <typename K, typename V>
void Dict<K, V>::InitializeTables() {
  Reset(0);
  Reset(1);
}

template <typename K, typename V>
void Dict<K, V>::InitializeTableWithSize(int i, int exp, size_t size) {
  table_size_exp_[i] = exp;
  tables_[i].resize(size);
  table_used_[i] = 0;
}

template <typename K, typename V>
void Dict<K, V>::InsertEntry(DictEntry* de, int i) {
  size_t key_idx = HashIndex(de->hash, i);
  de->next = tables_[i][key_idx];
  tables_[i][key_idx] = de;
  ++table_used_[i];
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::Unlink(const K& key) {
  RehashStepIfNeeded();
  size_t hash = KeyHash(key);
  for (size_t i = 0; i < tables_.size(); ++i) {
    if (tables_[i].empty()) {
      if (!IsRehashing()) {
        break;
      }
      continue;
    }
    size_t idx = HashIndex(hash, i);
    DictEntry *entry = tables_[i][idx], *prev = nullptr;
    while (entry) {
      if (entry->hash == hash && IsEqual(key, entry->key)) {
        UnlinkEntry(entry, prev, i);
        return entry;
      }
      prev = entry, entry = entry->next;
    }
    if (!IsRehashing()) {
      break;
    }
  }
  return nullptr;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::Unlink(std::string_view key) {
  static_assert(std::is_same<K, std::string>::value,
                "std::string_view unlink only supports std::string keys");
  RehashStepIfNeeded();
  const size_t hash = StringViewKeyHash(key);
  for (size_t table = 0; table < tables_.size(); ++table) {
    if (tables_[table].empty()) {
      if (!IsRehashing()) {
        break;
      }
      continue;
    }
    const size_t index = HashIndex(hash, table);
    DictEntry* entry = tables_[table][index];
    DictEntry* previous = nullptr;
    while (entry != nullptr) {
      if (entry->hash == hash && IsEqual(key, entry->key)) {
        UnlinkEntry(entry, previous, table);
        return entry;
      }
      previous = entry;
      entry = entry->next;
    }
    if (!IsRehashing()) {
      break;
    }
  }
  return nullptr;
}

template <typename K, typename V>
void Dict<K, V>::UnlinkEntry(DictEntry* de, DictEntry* prev, int i) {
  if (prev) {
    prev->next = de->next;
  } else {
    size_t key_idx = HashIndex(de->hash, i);
    tables_[i][key_idx] = de->next;
  }
  de->next = nullptr;
  --table_used_[i];
}

template <typename K, typename V>
void Dict<K, V>::DeleteEntry(DictEntry* de, DictEntry* prev, int i) {
  UnlinkEntry(de, prev, i);
  FreeUnlinkedEntry(de);
}

template <typename K, typename V>
size_t Dict<K, V>::TableMask(int i) const {
  return table_size_exp_[i] == -1 ? 0 : (size_t{1} << table_size_exp_[i]) - 1;
}

template <typename K, typename V>
size_t Dict<K, V>::HashIndex(size_t hash, int i) const {
  return hash & TableMask(i);
}

template <typename K, typename V>
size_t Dict<K, V>::KeyHash(const K& key) const {
  assert(type_.hash_function);
  return type_.hash_function(key);
}

template <typename K, typename V>
size_t Dict<K, V>::StringViewKeyHash(std::string_view key) const {
  static_assert(std::is_same<K, std::string>::value,
                "StringViewKeyHash only supports std::string keys");
  if (type_.string_view_hash_function) {
    return type_.string_view_hash_function(key);
  }
  assert(type_.hash_function);
  return type_.hash_function(std::string(key));
}

template <typename K, typename V>
int Dict<K, V>::NextExp(size_t size) const {
  int exponent = 1;
  size_t table_size = size_t{1} << exponent;
  while (table_size < size) {
    if (table_size > std::numeric_limits<size_t>::max() / 2) {
      return -1;
    }
    table_size *= 2;
    ++exponent;
  }
  return exponent;
}

template <typename K, typename V>
bool Dict<K, V>::IsEqual(const K& key1, const K& key2) const {
  return key1 == key2 ||
         (type_.key_compare && !(type_.key_compare(key1, key2)));
}

template <typename K, typename V>
bool Dict<K, V>::IsEqual(std::string_view key1, const K& key2) const {
  static_assert(std::is_same<K, std::string>::value,
                "string_view comparison only supports std::string keys");
  if (key1 == std::string_view(key2)) {
    return true;
  }
  if (type_.string_view_key_compare) {
    return !type_.string_view_key_compare(key1, key2);
  }
  if (type_.key_compare) {
    return !type_.key_compare(std::string(key1), key2);
  }
  return false;
}

template <typename K, typename V>
void Dict<K, V>::SetKey(DictEntry* entry, const K& key) {
  entry->key = type_.key_dup ? type_.key_dup(key) : key;
}

template <typename K, typename V>
void Dict<K, V>::SetKey(DictEntry* entry, K&& key) {
  entry->key = type_.key_dup ? type_.key_dup(key) : std::move(key);
}

template <typename K, typename V>
void Dict<K, V>::SetVal(DictEntry* entry, const V& val) {
  entry->val = type_.val_dup ? type_.val_dup(val) : val;
}

template <typename K, typename V>
void Dict<K, V>::SetVal(DictEntry* entry, V&& val) {
  entry->val = type_.val_dup ? type_.val_dup(val) : std::move(val);
}

template <typename K, typename V>
void Dict<K, V>::FreeKey(DictEntry* entry) {
  if (type_.key_destructor) {
    type_.key_destructor(entry->key);
  }
}

template <typename K, typename V>
void Dict<K, V>::FreeVal(DictEntry* entry) {
  if (type_.val_destructor) {
    type_.val_destructor(entry->val);
  }
}

template <typename K, typename V>
void Dict<K, V>::FreeUnlinkedEntry(DictEntry* entry) {
  if (entry) {
    FreeKey(entry);
    FreeVal(entry);
    delete entry;
  }
}

template <typename K, typename V>
std::optional<size_t> Dict<K, V>::KeyIndex(const K& key, size_t hash,
                                           Dict<K, V>::DictEntry** existing) {
  if (existing != nullptr) {
    *existing = nullptr;
  }
  size_t idx = 0;
  for (size_t i = 0; i < tables_.size(); ++i) {
    if (tables_[i].empty()) {
      if (!IsRehashing()) {
        break;
      }
      continue;
    }
    idx = HashIndex(hash, i);
    DictEntry* entry = tables_[i][idx];
    while (entry) {
      if (entry->hash == hash && IsEqual(entry->key, key)) {
        if (existing) {
          *existing = entry;
        }
        return std::nullopt;
      }
      entry = entry->next;
    }
    if (!IsRehashing()) {
      break;
    }
  }
  return idx;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::InsertRaw(
    const K& key, Dict<K, V>::DictEntry** existing) {
  ExpandIfNeeded();
  RehashStepIfNeeded();
  size_t hash = KeyHash(key);
  const auto idx = KeyIndex(key, hash, existing);
  if (!idx.has_value()) {
    return nullptr;
  }
  int i = IsRehashing() ? 1 : 0;
  auto entry = std::make_unique<DictEntry>();
  entry->hash = hash;
  SetKey(entry.get(), key);
  InsertEntry(entry.get(), i);
  return entry.release();
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::InsertRaw(
    K&& key, Dict<K, V>::DictEntry** existing) {
  ExpandIfNeeded();
  RehashStepIfNeeded();
  const size_t hash = KeyHash(key);
  const auto idx = KeyIndex(key, hash, existing);
  if (!idx.has_value()) {
    return nullptr;
  }
  const int table = IsRehashing() ? 1 : 0;
  auto entry = std::make_unique<DictEntry>();
  entry->hash = hash;
  SetKey(entry.get(), std::move(key));
  InsertEntry(entry.get(), table);
  return entry.release();
}

template <typename K, typename V>
void Dict<K, V>::ExpandIfNeeded() {
  const size_t table_size = TableSize(table_size_exp_[0]);
  if (table_size == 0 || static_cast<double>(table_used_[0]) / table_size >=
                             kDictForceResizeRatio) {
    Expand(table_used_[0] + 1);
  }
}

template <typename K, typename V>
bool Dict<K, V>::Expand(size_t size) {
  if (IsRehashing() || size < table_used_[0]) {
    return false;
  }
  int new_exp = NextExp(size);
  if (new_exp < 0) {
    return false;
  }
  if (new_exp <= table_size_exp_[0]) {
    return false;
  }
  size_t new_size = TableSize(new_exp);
  if (new_size < size ||
      new_size > std::numeric_limits<size_t>::max() / sizeof(DictEntry*)) {
    return false;
  }
  // First allocation initializes table 0; later expansions rehash into table 1.
  if (table_size_exp_[0] < 0) {
    InitializeTableWithSize(0, new_exp, new_size);
    rehash_idx_ = std::nullopt;
    return true;
  }
  InitializeTableWithSize(1, new_exp, new_size);
  rehash_idx_ = size_t{0};
  return true;
}

template <typename K, typename V>
void Dict<K, V>::RehashStepIfNeeded() {
  if (pause_rehash_ == 0) {
    Rehash(1);
  }
}

// Perform at most n non-empty bucket migrations. Empty buckets are skipped with
// a bounded visit count so sparse tables cannot stall one operation for too
// long.
template <typename K, typename V>
bool Dict<K, V>::Rehash(int n) {
  // Do nothing unless a second table is active.
  if (!IsRehashing()) {
    return false;
  }
  int empty_visit = n * 10;
  size_t dict_size = TableSize(table_size_exp_[0]);
  while (n > 0 && *rehash_idx_ < dict_size && table_used_[0] > 0 &&
         empty_visit > 0) {
    DictEntry* de = tables_[0][*rehash_idx_];
    if (!de) {
      --empty_visit;
      ++*rehash_idx_;
      continue;
    }
    while (de) {
      DictEntry* next = de->next;
      UnlinkEntry(de, nullptr, 0);
      InsertEntry(de, 1);
      de = next;
    }
    tables_[0][*rehash_idx_] = nullptr;
    --n;
    ++*rehash_idx_;
  }
  if (table_used_[0] == 0) {
    MigrateRehashedTable();
    return false;
  }
  return true;
}

template <typename K, typename V>
void Dict<K, V>::MigrateRehashedTable() {
  tables_[0] = std::move(tables_[1]);
  table_size_exp_[0] = table_size_exp_[1];
  table_used_[0] = table_used_[1];
  Reset(1);
  rehash_idx_ = std::nullopt;
}

/*
 * Helper function for dict clear. Delete all key-value pairs in the given table
 * and reset the table to the initial state.
 */
template <typename K, typename V>
void Dict<K, V>::Clear(int i) {
  for (size_t j = 0; j < tables_[i].size() && table_used_[i] > 0; ++j) {
    DictEntry* de = tables_[i][j];
    while (de) {
      DictEntry* next = de->next;
      DeleteEntry(de, nullptr, i);
      de = next;
    }
    tables_[i][j] = nullptr;
  }
  Reset(i);
}

template <typename K, typename V>
void Dict<K, V>::Reset(int i) {
  if (i < 0 || static_cast<size_t>(i) >= tables_.size()) {
    return;
  }
  tables_[i].clear();
  table_used_[i] = 0;
  table_size_exp_[i] = -1;
}
}  // namespace redis_simple::in_memory
