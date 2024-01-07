#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace redis_simple {
namespace in_memory {
template <typename K, typename V>
class Dict {
 public:
  /* Entry storing the key-value pair */
  struct DictEntry;
  /* specify key-value related functions */
  struct DictType;
  using dictScanFunc = void (*)(const DictEntry*);
  static std::unique_ptr<Dict<K, V>> Init();
  static std::unique_ptr<Dict<K, V>> Init(const DictType& type);
  size_t Size() { return ht_used_[0] + ht_used_[1]; }
  DictEntry* Find(const K& key);
  DictEntry* Find(K&& key);
  bool Add(const K& key, const V& val);
  bool Add(K&& key, V&& val);
  DictEntry* AddOrFind(const K& key);
  DictEntry* AddOrFind(K&& key);
  void Replace(const K& key, const V& val);
  void Replace(K&& key, V&& val);
  bool Delete(const K& key);
  bool Delete(K&& key);
  DictEntry* Unlink(const K& key);
  DictEntry* Unlink(K&& key);
  ssize_t Scan(size_t cursor, dictScanFunc callback);
  void Clear();
  ~Dict();

 private:
  Dict();
  Dict(const DictType& type);
  void InitTables();
  size_t HtSize(int exp) { return exp < 0 ? 0 : 1 << exp; }
  bool IsRehashing() { return rehash_idx_ >= 0; }
  void PauseRehashing() { ++pause_rehash_; }
  void ResumeRehashing() {
    if (pause_rehash_ > 0) --pause_rehash_;
  }
  size_t HtMask(int i);
  size_t KeyHashIndex(const K& key, int i);
  int NextExp(ssize_t size);
  bool IsEqual(const K& key1, const K& key2);
  void SetKey(DictEntry* entry, const K& key);
  void SetVal(DictEntry* entry, const V& val);
  void FreeKey(DictEntry* entry);
  void FreeVal(DictEntry* entry);
  void FreeUnlinkedEntry(DictEntry* entry);
  ssize_t KeyIndex(const K& key, DictEntry** existing);
  DictEntry* AddRaw(const K& key, DictEntry** existing);
  void ExpandIfNeeded();
  bool Expand(size_t size);
  void RehashStepIfNeeded();
  bool Rehash(int n);
  void Clear(int i);
  void Reset(int i);
  /* hashtable initial size */
  static constexpr const int htInitSize = 2;
  /* hashtable initial exponential */
  static constexpr const int htInitExp = 1;
  /* the threshold for rehashing. The ratio is calculated from (num of elements
   * / hashtable size) */
  static constexpr const double dictForceResizeRatio = 2.0;
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
 * Initialize the dict with default functions.
 */
template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Init() {
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>());
  if (!dict->Expand(htInitSize)) {
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
  if (!dict->Expand(htInitSize)) {
    return nullptr;
  }
  return dict;
}

/*
 * Find the entry containing the given key.
 */
template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::Find(const K& key) {
  RehashStepIfNeeded();
  for (size_t i = 0; i < ht_.size(); ++i) {
    size_t idx = KeyHashIndex(key, i);
    DictEntry* entry = ht_[i][idx];
    while (entry) {
      if (IsEqual(key, entry->key)) {
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
typename Dict<K, V>::DictEntry* Dict<K, V>::Find(K&& key) {
  return Find(key);
}

/*
 * Insert the key-value pair to the dict.
 * Return false if the key already exists in the dict.
 */
template <typename K, typename V>
bool Dict<K, V>::Add(const K& key, const V& val) {
  DictEntry* entry = AddRaw(key, nullptr);
  if (!entry) {
    return false;
  }
  SetVal(entry, val);
  return true;
}

template <typename K, typename V>
bool Dict<K, V>::Add(K&& key, V&& val) {
  return Add(key, val);
}

/*
 * Add or find the given key.
 * Return the newly created dict entry if the key is inserted into the dict.
 * Otherwise, return the existing dict entry containing the key-value pair.
 */
template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::AddOrFind(const K& key) {
  DictEntry* existing;
  DictEntry* new_entry = AddRaw(key, &existing);
  return new_entry ? new_entry : existing;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::AddOrFind(K&& key) {
  return AddOrFind(key);
}

/*
 * Replace the given key with the value.
 * Insert the key-value pair into the dict if the key is not found.
 */
template <typename K, typename V>
void Dict<K, V>::Replace(const K& key, const V& val) {
  DictEntry* existing;
  DictEntry* entry = AddRaw(key, &existing);
  if (entry) {
    SetVal(entry, val);
  } else {
    DictEntry auxentry = *existing;
    SetVal(existing, val);
    FreeVal(&auxentry);
  }
}

template <typename K, typename V>
void Dict<K, V>::Replace(K&& key, V&& val) {
  Replace(key, val);
}

/*
 * Delete the key from the dict, and free the memory of the entry used to store
 * the key-value pair.
 */
template <typename K, typename V>
bool Dict<K, V>::Delete(const K& key) {
  DictEntry* de = Unlink(key);
  FreeUnlinkedEntry(de);
  return de;
}

template <typename K, typename V>
bool Dict<K, V>::Delete(K&& key) {
  return Delete(key);
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
        if (prev) {
          prev->next = entry->next;
        } else {
          ht_[i][idx] = entry->next;
        }
        entry->next = nullptr;
        --ht_used_[i];
        return entry;
      }
      prev = entry, entry = entry->next;
    }
    if (!IsRehashing()) {
      break;
    }
  }
  return nullptr;
  ;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::Unlink(K&& key) {
  return Unlink(key);
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
      callback(de);
      de = next;
    }
  }
  /* if a rehashing is in progress, scan all dict entries at the corresponding
   * index in the rehashed table */
  if (IsRehashing() && cursor < ht_[1].size()) {
    const DictEntry* de = ht_[1][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de);
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

/*
 * Return the mask of the given table which is used to calculate the hash index
 * of the key.
 */
template <typename K, typename V>
size_t Dict<K, V>::HtMask(int i) {
  return ht_size_exp_[i] == -1 ? 0 : (1 << ht_size_exp_[i]) - 1;
}

/*
 * Return the hash index of the key.
 */
template <typename K, typename V>
size_t Dict<K, V>::KeyHashIndex(const K& key, int i) {
  assert(type_.hashFunction);
  return (type_.hashFunction(key)) & HtMask(i);
}

/*
 * Return the exponential that is greater or equal to the given size.
 * The function is called in by DictExpand.
 */
template <typename K, typename V>
int Dict<K, V>::NextExp(ssize_t size) {
  if (size < 0) return htInitExp;
  int i = 1;
  while ((1 << i) < size) ++i;
  return i;
}

template <typename K, typename V>
bool Dict<K, V>::IsEqual(const K& key1, const K& key2) {
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
 * Return the hash index to store the key.
 * If the key already exists, make `existing` point to the entry.
 * The function is called everytime when a key is trying to be inserted into the
 * dict.
 */
template <typename K, typename V>
ssize_t Dict<K, V>::KeyIndex(const K& key, Dict<K, V>::DictEntry** existing) {
  if (existing) {
    *existing = nullptr;
  }
  ExpandIfNeeded();
  ssize_t idx = -1;
  for (size_t i = 0; i < ht_.size(); ++i) {
    idx = KeyHashIndex(key, i);
    DictEntry* entry = ht_[i][idx];
    while (entry) {
      if (IsEqual(entry->key, key)) {
        if (existing) {
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
typename Dict<K, V>::DictEntry* Dict<K, V>::AddRaw(
    const K& key, Dict<K, V>::DictEntry** existing) {
  RehashStepIfNeeded();
  ssize_t idx = KeyIndex(key, existing);
  if (idx < 0) {
    return nullptr;
  }
  /* if a rehashing is in progress, directly insert the key to the rehashed
   * table
   */
  int i = IsRehashing() ? 1 : 0;
  DictEntry* de = new DictEntry();
  SetKey(de, key);
  de->next = ht_[i][idx];
  ht_[i][idx] = de;
  ++ht_used_[i];
  return de;
}

/*
 * Expand the dict if ratio of the number of key-value pairs stored to the dict
 * size exceeds the threshold.
 */
template <typename K, typename V>
void Dict<K, V>::ExpandIfNeeded() {
  if ((double)ht_used_[0] / HtSize(ht_size_exp_[0]) >= dictForceResizeRatio) {
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
    ht_size_exp_[0] = new_exp;
    ht_[0].resize(new_size);
    ht_used_[0] = 0;
    rehash_idx_ = -1;
    return true;
  }
  /* initialize the rehashed table */
  ht_size_exp_[1] = new_exp;
  ht_[1].resize(new_size);
  ht_used_[1] = 0;
  rehash_idx_ = 0;
  return true;
}

/*
 * If a rehashing is in progress, perform rehash for at most one bucket. Called
 * in Add, Update, Find, Delete.
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
  while (n > 0 && rehash_idx_ < dict_size && ht_used_[0] && empty_visit) {
    DictEntry* de = ht_[0][rehash_idx_];
    if (!de) {
      --empty_visit, ++rehash_idx_;
      continue;
    }
    while (de) {
      size_t idx = KeyHashIndex(de->key, 1);
      DictEntry* next = de->next;
      de->next = ht_[1][idx];
      ht_[1][idx] = de;
      --ht_used_[0], ++ht_used_[1];
      de = next;
    }
    ht_[0][rehash_idx_] = nullptr;
    --n, ++rehash_idx_;
  }
  /* rehash completes, copy all the states of the table 1 to table 0 and reset
   * the table 1 to the initial state. */
  if (ht_used_[0] == 0) {
    ht_[0] = std::move(ht_[1]);
    ht_size_exp_[0] = ht_size_exp_[1];
    ht_used_[0] = ht_used_[1];
    Reset(1);
    rehash_idx_ = -1;
    return false;
  }
  /* still exists keys to rehash */
  return true;
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
      de->next = nullptr;
      FreeUnlinkedEntry(de);
      de = next;
      --ht_used_[i];
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
