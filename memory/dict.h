#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace redis_simple::in_memory {
template <typename K, typename V>
class Dict {
 public:
  class Iterator;
  struct DictType;
  using DictScanFunc = void (*)(const K& key, const V& value);
  static std::unique_ptr<Dict<K, V>> Init();
  static std::unique_ptr<Dict<K, V>> Init(size_t capacity);
  static std::unique_ptr<Dict<K, V>> Init(const DictType& type);
  std::optional<V> Get(const K& key);
  std::optional<V> Get(K&& key);
  void Set(const K& key, const V& val);
  void Set(K&& key, V&& val);
  bool Insert(const K& key, const V& val);
  bool Insert(K&& key, V&& val);
  bool Delete(const K& key);
  bool Delete(K&& key);
  ssize_t Scan(size_t cursor, DictScanFunc callback);
  size_t Size() const { return table_used_[0] + table_used_[1]; }
  void Clear();
  ~Dict();

 private:
  struct DictEntry;
  Dict();
  Dict(const DictType& type);
  void InitTables();
  void InitTableWithSize(int i, int exp, size_t size);
  void InsertEntry(DictEntry* entry, int i);
  DictEntry* Unlink(const K& key);
  void UnlinkEntry(DictEntry* entry, DictEntry* prev, int i);
  void DeleteEntry(DictEntry* entry, DictEntry* prev, int i);
  size_t TableSize(int exp) const { return exp < 0 ? 0 : 1 << exp; }
  bool IsRehashing() const { return rehash_idx_ >= 0; }
  void PauseRehashing() { ++pause_rehash_; }
  void ResumeRehashing() {
    if (pause_rehash_ > 0) --pause_rehash_;
  }
  size_t TableMask(int i) const;
  size_t HashIndex(size_t hash, int i) const;
  size_t KeyHash(const K& key) const;
  int NextExp(ssize_t size) const;
  bool IsEqual(const K& key1, const K& key2) const;
  void SetKey(DictEntry* entry, const K& key);
  void SetVal(DictEntry* entry, const V& val);
  void SetVal(DictEntry* entry, V&& val);
  void FreeKey(DictEntry* entry);
  void FreeVal(DictEntry* entry);
  void FreeUnlinkedEntry(DictEntry* entry);
  ssize_t KeyIndex(const K& key, size_t hash, DictEntry** existing);
  DictEntry* InsertRaw(const K& key, DictEntry** existing);
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
  std::vector<std::vector<DictEntry*>> tables_;
  size_t table_used_[2];
  int table_size_exp_[2];
  ssize_t rehash_idx_;
  size_t pause_rehash_;
};

template <typename K, typename V>
struct Dict<K, V>::DictEntry {
  DictEntry() : hash(0), next(nullptr) {}
  K key;
  V val;
  size_t hash;
  DictEntry* next;
};

template <typename K, typename V>
struct Dict<K, V>::DictType {
  // Used to get the hash index of the key. Use std::hash by default.
  std::function<const size_t(const K& key)> hash_function;
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
  explicit Iterator(const Dict* dict, const DictEntry* entry);
  Iterator& operator=(const Iterator& it);
  bool operator==(const Iterator& it);
  bool operator!=(const Iterator& it);
  bool Valid() const;
  void SeekToFirst();
  void SeekToLast();
  void Next();
  void operator++();
  K Key() { return entry_->key; }
  V Value() { return entry_->val; }

 private:
  void SeekToNextEntry();
  const Dict* dict_;
  int table_;
  ssize_t idx_;
  const DictEntry* entry_;
};

template <typename K, typename V>
Dict<K, V>::Iterator::Iterator(const Dict* dict)
    : dict_(dict), entry_(nullptr), table_(0), idx_(-1) {}

template <typename K, typename V>
Dict<K, V>::Iterator::Iterator(const Dict* dict, const DictEntry* entry)
    : dict_(dict), entry_(entry), table_(0), idx_(-1) {}

template <typename K, typename V>
typename Dict<K, V>::Iterator& Dict<K, V>::Iterator::operator=(
    const Iterator& it) {
  dict_ = it.dict_;
  table_ = it.table_;
  idx_ = it.idx_;
  entry_ = it.entry_;
  return *this;
}

template <typename K, typename V>
bool Dict<K, V>::Iterator::operator==(const Iterator& it) {
  return dict_ == it.dict_ && table_ == it.table_ && idx_ == it.idx_ &&
         entry_ == it.entry_;
}

template <typename K, typename V>
bool Dict<K, V>::Iterator::operator!=(const Iterator& it) {
  return !((*this) == it);
}

template <typename K, typename V>
bool Dict<K, V>::Iterator::Valid() const {
  return entry_ != nullptr;
}

template <typename K, typename V>
void Dict<K, V>::Iterator::SeekToFirst() {
  table_ = 0;
  idx_ = -1;
  entry_ = nullptr;
  SeekToNextEntry();
}

template <typename K, typename V>
void Dict<K, V>::Iterator::SeekToLast() {
  if (dict_->Size() == 0) {
    return;
  }
  if (dict_->rehash_idx_ > 0) {
    table_ = 1;
  }
  idx_ = dict_->tables_[table_].size() - 1;
  while (idx_ >= 0 && !dict_->tables_[table_][idx_]) {
    --idx_;
  }
  entry_ = dict_->tables_[table_][idx_];
  while (entry_->next) {
    entry_ = entry_->next;
  }
}

template <typename K, typename V>
void Dict<K, V>::Iterator::operator++() {
  SeekToNextEntry();
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
  ++idx_;
  // Find next non empty table entry list.
  while (idx_ < dict_->tables_[table_].size() &&
         !dict_->tables_[table_][idx_]) {
    ++idx_;
  }
  if (idx_ < dict_->tables_[table_].size()) {
    entry_ = dict_->tables_[table_][idx_];
    return;
  }
  if (table_ == 1 || dict_->rehash_idx_ <= 0) {
    entry_ = nullptr;
    return;
  }
  // Find the next element in the rehashed table.
  idx_ = -1;
  table_ = 1;
  SeekToNextEntry();
}

/*
 * Initialize the dict with default functions.
 */
template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Init() {
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>());
  if (!dict->Expand(kTableInitSize)) {
    return nullptr;
  }
  dict->type_.hash_function = [](const K& key) {
    std::hash<K> h;
    return h(key);
  };
  return dict;
}

/*
 * Initialize the dict with default functions and custom capacity.
 */
template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Init(size_t capacity) {
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>());
  if (!dict->Expand(capacity)) {
    return nullptr;
  }
  dict->type_.hash_function = [](const K& key) {
    std::hash<K> h;
    return h(key);
  };
  return dict;
}

/*
 * Initialize the dict with customized functions.
 */
template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Init(
    const typename Dict<K, V>::DictType& type) {
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
  RehashStepIfNeeded();
  size_t hash = KeyHash(key);
  for (size_t i = 0; i < tables_.size(); ++i) {
    size_t idx = HashIndex(hash, i);
    DictEntry* entry = tables_[i][idx];
    while (entry) {
      if (entry->hash == hash && IsEqual(key, entry->key)) {
        return entry->val;
      }
      entry = entry->next;
    }
    if (!IsRehashing()) {
      break;
    }
  }
  return std::nullopt;
}

template <typename K, typename V>
typename std::optional<V> Dict<K, V>::Get(K&& key) {
  return Get(key);
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
void Dict<K, V>::Set(K&& key, V&& val) {
  DictEntry* existing = nullptr;
  DictEntry* entry = InsertRaw(key, &existing);
  if (entry) {
    SetVal(entry, std::move(val));
  } else if (existing) {
    DictEntry auxentry = *existing;
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
  DictEntry* entry = InsertRaw(key, nullptr);
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
bool Dict<K, V>::Delete(K&& key) {
  return Delete(key);
}

template <typename K, typename V>
ssize_t Dict<K, V>::Scan(size_t cursor, DictScanFunc callback) {
  // Scanning visits the same bucket index in both tables; pause incremental
  // rehashing so entries do not move while this cursor position is processed.
  PauseRehashing();
  if (cursor < tables_[0].size()) {
    const DictEntry* de = tables_[0][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de->key, de->val);
      de = next;
    }
  }
  // During rehashing, the same cursor position can have entries in both tables.
  if (IsRehashing() && cursor < tables_[1].size()) {
    const DictEntry* de = tables_[1][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de->key, de->val);
      de = next;
    }
  }
  if (++cursor >= std::max(tables_[0].size(), tables_[1].size())) {
    cursor = -1;
  }
  ResumeRehashing();
  return cursor;
}

template <typename K, typename V>
void Dict<K, V>::Clear() {
  Clear(0);
  Clear(1);
  rehash_idx_ = -1;
  pause_rehash_ = 0;
}

template <typename K, typename V>
Dict<K, V>::~Dict() {
  Clear();
}

template <typename K, typename V>
Dict<K, V>::Dict() : rehash_idx_(-1), pause_rehash_(0) {
  InitTables();
}

template <typename K, typename V>
Dict<K, V>::Dict(const DictType& type)
    : rehash_idx_(-1), pause_rehash_(0), type_(type) {
  InitTables();
}

template <typename K, typename V>
void Dict<K, V>::InitTables() {
  tables_.resize(2);
  Reset(0);
  Reset(1);
}

template <typename K, typename V>
void Dict<K, V>::InitTableWithSize(int i, int exp, size_t size) {
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
  return table_size_exp_[i] == -1 ? 0 : (1 << table_size_exp_[i]) - 1;
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
int Dict<K, V>::NextExp(ssize_t size) const {
  if (size < 0) return kTableInitExp;
  int i = 1;
  while ((1 << i) < size) ++i;
  return i;
}

template <typename K, typename V>
bool Dict<K, V>::IsEqual(const K& key1, const K& key2) const {
  return key1 == key2 ||
         (type_.key_compare && !(type_.key_compare(key1, key2)));
}

template <typename K, typename V>
void Dict<K, V>::SetKey(DictEntry* entry, const K& key) {
  entry->key = type_.key_dup ? type_.key_dup(key) : key;
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
    entry = nullptr;
  }
}

template <typename K, typename V>
ssize_t Dict<K, V>::KeyIndex(const K& key, size_t hash,
                             Dict<K, V>::DictEntry** existing) {
  if (existing) *existing = nullptr;
  ssize_t idx = -1;
  for (size_t i = 0; i < tables_.size(); ++i) {
    idx = HashIndex(hash, i);
    DictEntry* entry = tables_[i][idx];
    while (entry) {
      if (entry->hash == hash && IsEqual(entry->key, key)) {
        if (existing) {
          *existing = entry;
        }
        return -1;
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
  ssize_t idx = KeyIndex(key, hash, existing);
  if (idx < 0) {
    return nullptr;
  }
  int i = IsRehashing() ? 1 : 0;
  DictEntry* de = new DictEntry();
  de->hash = hash;
  SetKey(de, key);
  InsertEntry(de, i);
  return de;
}

template <typename K, typename V>
void Dict<K, V>::ExpandIfNeeded() {
  if ((double)table_used_[0] / TableSize(table_size_exp_[0]) >=
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
  if (new_exp <= table_size_exp_[0]) {
    return false;
  }
  size_t new_size = TableSize(new_exp);
  if (new_size < size ||
      new_size * sizeof(DictEntry*) < size * sizeof(DictEntry*)) {
    return false;
  }
  // First allocation initializes table 0; later expansions rehash into table 1.
  if (table_size_exp_[0] < 0) {
    InitTableWithSize(0, new_exp, new_size);
    rehash_idx_ = -1;
    return true;
  }
  InitTableWithSize(1, new_exp, new_size);
  rehash_idx_ = 0;
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
  while (n > 0 && rehash_idx_ < dict_size && table_used_[0] > 0 &&
         empty_visit > 0) {
    DictEntry* de = tables_[0][rehash_idx_];
    if (!de) {
      --empty_visit, ++rehash_idx_;
      continue;
    }
    while (de) {
      DictEntry* next = de->next;
      UnlinkEntry(de, nullptr, 0);
      InsertEntry(de, 1);
      de = next;
    }
    tables_[0][rehash_idx_] = nullptr;
    --n, ++rehash_idx_;
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
  rehash_idx_ = -1;
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
  if (i >= tables_.size()) {
    return;
  }
  tables_[i].clear();
  table_used_[i] = 0;
  table_size_exp_[i] = -1;
}
}  // namespace redis_simple::in_memory
