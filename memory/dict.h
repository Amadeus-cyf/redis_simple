#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace redis_simple {
namespace in_memory {
template <typename K, typename V>
class Dict {
 public:
  class Iterator;
  /* specify key-value related functions */
  struct DictType;
  using dictScanFunc = void (*)(const K& key, const V& value);
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
  ssize_t Scan(size_t cursor, dictScanFunc callback);
  size_t Size() const { return ht_used_[0] + ht_used_[1]; }
  void Clear();
  ~Dict();

 private:
  /* Entry storing the key-value pair */
  struct DictEntry;
  Dict();
  Dict(const DictType& type);
  void InitTables();
  void InitTableWithSize(int i, int exp, size_t size);
  void InsertEntry(DictEntry* entry, int i);
  DictEntry* Unlink(const K& key);
  void UnlinkEntry(DictEntry* entry, DictEntry* prev, int i);
  void DeleteEntry(DictEntry* entry, DictEntry* prev, int i);
  size_t HtSize(int exp) const { return exp < 0 ? 0 : 1 << exp; }
  bool IsRehashing() const { return rehash_idx_ >= 0; }
  void PauseRehashing() { ++pause_rehash_; }
  void ResumeRehashing() {
    if (pause_rehash_ > 0) --pause_rehash_;
  }
  size_t HtMask(int i) const;
  size_t KeyHashIndex(const K& key, int i) const;
  int NextExp(ssize_t size) const;
  bool IsEqual(const K& key1, const K& key2) const;
  void SetKey(DictEntry* entry, const K& key);
  void SetVal(DictEntry* entry, const V& val);
  void FreeKey(DictEntry* entry);
  void FreeVal(DictEntry* entry);
  void FreeUnlinkedEntry(DictEntry* entry);
  ssize_t KeyIndex(const K& key, DictEntry** existing);
  DictEntry* InsertRaw(const K& key, DictEntry** existing);
  void ExpandIfNeeded();
  bool Expand(size_t size);
  void RehashStepIfNeeded();
  void MigrateRehashedTable();
  bool Rehash(int n);
  void Clear(int i);
  void Reset(int i);
  /* hashtable initial size */
  static constexpr const int HtInitSize = 2;
  /* hashtable initial exponential */
  static constexpr const int HtInitExp = 1;
  /* the threshold for rehashing. The ratio is calculated from (num of elements
   * / hashtable size) */
  static constexpr const double DictForceResizeRatio = 2.0;
  /* specify hash function, key constructor and destructor in type */
  DictType type_;
  /* containing 2 hastables, the second one is used for rehash */
  std::vector<std::vector<DictEntry*>> ht_;
  /* number of elements in each hashtable */
  size_t ht_used_[2];
  /* exponential used to calculate the size of the 2 hashtables */
  int ht_size_exp_[2];
  /* the idx that the rehash will start at, -1 if no rehash happened */
  ssize_t rehash_idx_;
  /* larger than 0 if rehash is paused */
  size_t pause_rehash_;
};

template <typename K, typename V>
struct Dict<K, V>::DictEntry {
  DictEntry() : next(nullptr){};
  K key;
  V val;
  DictEntry* next;
};

template <typename K, typename V>
struct Dict<K, V>::DictType {
  /* used to get the hash index of the key. Use std::hash by default. */
  std::function<const size_t(const K& key)> hashFunction;
  /* if set, all keys will be copied while being inserted into the dict. */
  std::function<K(const K& key)> keyDup;
  /* if set, all values will be copied while being inserted into the dict. */
  std::function<V(const V& val)> valDup;
  /* callback function when the key is freed from the dict. */
  std::function<void(K& key)> keyDestructor;
  /* callback function when the value is freed from the dict. */
  std::function<void(V& val)> valDestructor;
  /* comparator for keys. Used to check whether two keys are equal. Use operator
   * = by default.*/
  std::function<int(const K& key1, const K& key2)> keyCompare;
};

/*
 * Dict Iterator
 */
template <typename K, typename V>
class Dict<K, V>::Iterator {
 public:
  explicit Iterator(const Dict* dict);
  explicit Iterator(const Dict* dict, const DictEntry* entry);
  Iterator& operator=(const Iterator& it);
  bool operator==(const Iterator& it);
  bool operator!=(const Iterator& it);
  /* Returns true if the iterator is positioned to a valid entry */
  bool Valid() const;
  /* Position to the first entry in the dict */
  void SeekToFirst();
  /* Position to the Last entry in the dict */
  void SeekToLast();
  /* Advance to the next entry */
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
  table_ = it.table;
  idx_ = it.idx_;
  entry_ = it.entry_;
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
  idx_ = dict_->ht_[table_].size() - 1;
  while (idx_ >= 0 && !dict_->ht_[table_][idx_]) {
    --idx_;
  }
  entry_ = dict_->ht_[table_][idx_];
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
  /* find next non empty table entry list */
  while (idx_ < dict_->ht_[table_].size() && !dict_->ht_[table_][idx_]) {
    ++idx_;
  }
  if (idx_ < dict_->ht_[table_].size()) {
    entry_ = dict_->ht_[table_][idx_];
    return;
  }
  if (table_ == 1 || dict_->rehash_idx_ <= 0) {
    entry_ = nullptr;
    return;
  }
  /* find the next element in the rehashed table */
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
  if (!dict->Expand(HtInitSize)) {
    return nullptr;
  }
  dict->type_.hashFunction = [](const K& key) {
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
  dict->type_.hashFunction = [](const K& key) {
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
  if (!dict->Expand(HtInitSize)) {
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
  for (size_t i = 0; i < ht_.size(); ++i) {
    size_t idx = KeyHashIndex(key, i);
    DictEntry* entry = ht_[i][idx];
    while (entry) {
      if (IsEqual(key, entry->key)) {
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
  DictEntry* existing;
  DictEntry* entry = InsertRaw(key, &existing);
  if (entry) {
    SetVal(entry, val);
  } else {
    DictEntry auxentry = *existing;
    SetVal(existing, val);
    FreeVal(&auxentry);
  }
}

template <typename K, typename V>
void Dict<K, V>::Set(K&& key, V&& val) {
  Set(key, val);
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
  return Insert(key, val);
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

/*
 * Scan dict with the given cursor and apply the callback function to
 * all scanned entries.
 * Return the cursor used for the next scanning. If all entries in the dict has
 * been scanned, return -1. The initial cursor is 0,.
 */
template <typename K, typename V>
ssize_t Dict<K, V>::Scan(size_t cursor, dictScanFunc callback) {
  PauseRehashing();
  if (cursor < ht_[0].size()) {
    const DictEntry* de = ht_[0][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de->key, de->val);
      de = next;
    }
  }
  /* if a rehashing is in progress, scan all dict entries at the corresponding
   * index in the rehashed table */
  if (IsRehashing() && cursor < ht_[1].size()) {
    const DictEntry* de = ht_[1][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de->key, de->val);
      de = next;
    }
  }
  /* scan finished, return -1 */
  if (++cursor >= std::max(ht_[0].size(), ht_[1].size())) {
    cursor = -1;
  }
  ResumeRehashing();
  return cursor;
}

/*
 * Delete all key-value pairs in the dict and reset the dict to the initial
 * state.
 */
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

/*
 * Initialize 2 hash tables in the dict.
 */
template <typename K, typename V>
void Dict<K, V>::InitTables() {
  /* init 2 tables, table 0 is the current table and table 1 is used for
   * rehashing */
  ht_.resize(2);
  /* init the table 0 */
  Reset(0);
  /* init the table 1 used for rehashing */
  Reset(1);
}

template <typename K, typename V>
void Dict<K, V>::InitTableWithSize(int i, int exp, size_t size) {
  ht_size_exp_[i] = exp;
  ht_[i].resize(size);
  ht_used_[i] = 0;
}

/**
 * Insert the Dict Entry into the hash table.
 * The function assumes the dict entry has the key set and the key is not
 *duplicated. Should call SetKey and KeyIndex before calling this function.
 *
 * @param entry the entry to insert.
 * @param i the hash table index, 0 or 1.
 */
template <typename K, typename V>
void Dict<K, V>::InsertEntry(DictEntry* de, int i) {
  size_t key_idx = KeyHashIndex(de->key, i);
  de->next = ht_[i][key_idx];
  ht_[i][key_idx] = de;
  ++ht_used_[i];
}

/*
 * Unlink the key from the list and returned the entry.
 * Unlike Delete, the function does not free the memory of the entry used to
 * store the key-value pair.
 */
template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::Unlink(const K& key) {
  RehashStepIfNeeded();
  for (size_t i = 0; i < ht_.size(); ++i) {
    size_t idx = KeyHashIndex(key, i);
    DictEntry *entry = ht_[i][idx], *prev = nullptr;
    while (entry) {
      if (IsEqual(key, entry->key)) {
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

/**
 * Unlink the entry from the hash table.
 * The function does not free the entry. Should call FreeUnlinkedEntry.
 *
 * @param entry the entry to unlink.
 * @param entry its previous entry.
 * @param i the hash table index, 0 or 1.
 */
template <typename K, typename V>
void Dict<K, V>::UnlinkEntry(DictEntry* de, DictEntry* prev, int i) {
  if (prev) {
    prev->next = de->next;
  } else {
    size_t key_idx = KeyHashIndex(de->key, i);
    ht_[i][key_idx] = de->next;
  }
  de->next = nullptr;
  --ht_used_[i];
}

/**
 * Delete the entry from the hash table and frees the memory.
 *
 * @param entry the entry to unlink.
 * @param entry its previous entry.
 * @param i the hash table index, 0 or 1.
 */
template <typename K, typename V>
void Dict<K, V>::DeleteEntry(DictEntry* de, DictEntry* prev, int i) {
  UnlinkEntry(de, prev, i);
  FreeUnlinkedEntry(de);
}

/*
 * Return the mask of the given table which is used to calculate the hash index
 * of the key.
 */
template <typename K, typename V>
size_t Dict<K, V>::HtMask(int i) const {
  return ht_size_exp_[i] == -1 ? 0 : (1 << ht_size_exp_[i]) - 1;
}

/*
 * Return the hash index of the key.
 */
template <typename K, typename V>
size_t Dict<K, V>::KeyHashIndex(const K& key, int i) const {
  assert(type_.hashFunction);
  return (type_.hashFunction(key)) & HtMask(i);
}

/*
 * Return the exponential that is greater or equal to the given size.
 * The function is called in by DictExpand.
 */
template <typename K, typename V>
int Dict<K, V>::NextExp(ssize_t size) const {
  if (size < 0) return HtInitExp;
  int i = 1;
  while ((1 << i) < size) ++i;
  return i;
}

template <typename K, typename V>
bool Dict<K, V>::IsEqual(const K& key1, const K& key2) const {
  return key1 == key2 || (type_.keyCompare && !(type_.keyCompare(key1, key2)));
}

template <typename K, typename V>
void Dict<K, V>::SetKey(DictEntry* entry, const K& key) {
  entry->key = type_.keyDup ? type_.keyDup(key) : key;
}

template <typename K, typename V>
void Dict<K, V>::SetVal(DictEntry* entry, const V& val) {
  entry->val = type_.valDup ? type_.valDup(val) : val;
}

template <typename K, typename V>
void Dict<K, V>::FreeKey(DictEntry* entry) {
  if (type_.keyDestructor) {
    type_.keyDestructor(entry->key);
  }
}

template <typename K, typename V>
void Dict<K, V>::FreeVal(DictEntry* entry) {
  if (type_.valDestructor) {
    type_.valDestructor(entry->val);
  }
}

/*
 * Delete the unlinked entry through calling key/value destructors and freeing
 * the memory.
 */
template <typename K, typename V>
void Dict<K, V>::FreeUnlinkedEntry(DictEntry* entry) {
  if (entry) {
    FreeKey(entry);
    FreeVal(entry);
    delete entry;
    entry = nullptr;
  }
}

/*
 * Return the hash index to store the key, -1 if key already exists.
 * If the key already exists and existing is set, make the it point to the entry
 * containing the key. The function is called everytime when a key is trying to
 * be inserted into the dict.
 */
template <typename K, typename V>
ssize_t Dict<K, V>::KeyIndex(const K& key, Dict<K, V>::DictEntry** existing) {
  if (existing) *existing = nullptr;
  ssize_t idx = -1;
  for (size_t i = 0; i < ht_.size(); ++i) {
    idx = KeyHashIndex(key, i);
    DictEntry* entry = ht_[i][idx];
    while (entry) {
      if (IsEqual(entry->key, key)) {
        if (existing) {
          /* key already exists in the dict */
          *existing = entry;
          return -1;
        }
      }
      entry = entry->next;
    }
    if (!IsRehashing()) {
      break;
    }
  }
  return idx;
}

/*
 * Helper function for inserting a key into the dict.
 * If the key already exists, make `existing` point to the entry.
 */
template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::InsertRaw(
    const K& key, Dict<K, V>::DictEntry** existing) {
  /* check if the dict needs to be expanded */
  ExpandIfNeeded();
  /* perform a single step of rehash if rehashing is in progress */
  RehashStepIfNeeded();
  ssize_t idx = KeyIndex(key, existing);
  /* key already exists */
  if (idx < 0) {
    return nullptr;
  }
  /* if a rehashing is in progress, directly insert the key to the rehashed
   * table
   */
  int i = IsRehashing() ? 1 : 0;
  DictEntry* de = new DictEntry();
  SetKey(de, key);
  InsertEntry(de, i);
  return de;
}

/*
 * Expand the dict if ratio of the number of key-value pairs stored to the dict
 * size exceeds the threshold.
 */
template <typename K, typename V>
void Dict<K, V>::ExpandIfNeeded() {
  if ((double)ht_used_[0] / HtSize(ht_size_exp_[0]) >= DictForceResizeRatio) {
    Expand(ht_used_[0] + 1);
  }
}

/*
 * Expand the dict to be greater or equal to the given size.
 * Return true if succeeded.
 */
template <typename K, typename V>
bool Dict<K, V>::Expand(size_t size) {
  /* do not expand the dict if a rehashing is in progress */
  if (IsRehashing() || size < ht_used_[0]) {
    return false;
  }
  int new_exp = NextExp(size);
  /* fail to expand if the new exponential value is less or equal to the current
   * one */
  if (new_exp <= ht_size_exp_[0]) {
    return false;
  }
  /* get the new dict size */
  size_t new_size = HtSize(new_exp);
  printf("dict expand to %zu %d\n", new_size, new_exp);
  /* check for overflow */
  if (new_size < size ||
      new_size * sizeof(DictEntry*) < size * sizeof(DictEntry*)) {
    return false;
  }
  /* Set the table 0 if it has not been initialized. This happened when the dict
   * has just been created and its size has not been initialized. */
  if (ht_size_exp_[0] < 0) {
    InitTableWithSize(0, new_exp, new_size);
    rehash_idx_ = -1;
    return true;
  }
  /* initialize the rehashed table */
  InitTableWithSize(1, new_exp, new_size);
  rehash_idx_ = 0;
  return true;
}

/*
 * If a rehashing is in progress, perform rehash for at most one bucket. Called
 * in Insert, Update, Find, Delete.
 */
template <typename K, typename V>
void Dict<K, V>::RehashStepIfNeeded() {
  if (pause_rehash_ == 0) {
    Rehash(1);
  }
}

/* Perform n steps of rehashing. Returns true if the rehashing is still not
 * completed and vice versa */
template <typename K, typename V>
bool Dict<K, V>::Rehash(int n) {
  /* do not rehash if a rehashing is in progress */
  if (!IsRehashing()) {
    return false;
  }
  int empty_visit = n * 10;
  size_t dict_size = HtSize(ht_size_exp_[0]);
  while (n > 0 && rehash_idx_ < dict_size && ht_used_[0] > 0 &&
         empty_visit > 0) {
    DictEntry* de = ht_[0][rehash_idx_];
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
    ht_[0][rehash_idx_] = nullptr;
    --n, ++rehash_idx_;
  }
  /* rehash completes, copy all the states of the table 1 to table 0 and reset
   * the table 1 to the initial state. */
  if (ht_used_[0] == 0) {
    MigrateRehashedTable();
    return false;
  }
  /* still exists keys to rehash */
  return true;
}

/**
 * Migrate the rehashed table to the current table. Called when the rehashing
 * completes.
 */
template <typename K, typename V>
void Dict<K, V>::MigrateRehashedTable() {
  ht_[0] = std::move(ht_[1]);
  ht_size_exp_[0] = ht_size_exp_[1];
  ht_used_[0] = ht_used_[1];
  Reset(1);
  rehash_idx_ = -1;
}

/*
 * Helper function for dict clear. Delete all key-value pairs in the given table
 * and reset the table to the initial state.
 */
template <typename K, typename V>
void Dict<K, V>::Clear(int i) {
  for (size_t j = 0; j < ht_[i].size() && ht_used_[i] > 0; ++j) {
    DictEntry* de = ht_[i][j];
    while (de) {
      DictEntry* next = de->next;
      DeleteEntry(de, nullptr, i);
      de = next;
    }
    ht_[i][j] = nullptr;
  }
  Reset(i);
}

template <typename K, typename V>
void Dict<K, V>::Reset(int i) {
  if (i >= ht_.size()) {
    return;
  }
  ht_[i].clear();
  ht_used_[i] = 0;
  ht_size_exp_[i] = -1;
}
}  // namespace in_memory
}  // namespace redis_simple
