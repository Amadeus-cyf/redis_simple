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
  struct DictEntry;
  struct DictType;
  using dictScanFunc = void (*)(const DictEntry*);
  static std::unique_ptr<Dict<K, V>> Init();
  static std::unique_ptr<Dict<K, V>> Init(const DictType* type);
  DictType* GetType() { return type_; }
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
  int Scan(int cursor, dictScanFunc callback);
  void Clear();
  ~Dict();

 private:
  Dict();
  size_t HtSize(int exp) { return exp < 0 ? 0 : 1 << exp; }
  bool IsRehashing() { return rehash_idx_ >= 0; }
  void PauseRehashing() { ++pause_rehash_; }
  void ResumeRehashing() {
    if (pause_rehash_ > 0) --pause_rehash_;
  }
  unsigned int HtMask(int i);
  unsigned int KeyHashIndex(const K& key, int i);
  unsigned int NextExp(int val);
  bool IsEqual(const K& key1, const K& key2);
  void SetKey(DictEntry* entry, const K& key);
  void SetVal(DictEntry* entry, const V& val);
  void FreeKey(DictEntry* entry);
  void FreeVal(DictEntry* entry);
  void FreeUnlinkedEntry(DictEntry* entry);
  unsigned int KeyIndex(const K& key, DictEntry** existing);
  DictEntry* AddRaw(const K& key, DictEntry** existing);
  void ExpandIfNeeded();
  bool Expand(size_t size);
  void RehashStep();
  bool Rehash(int n);
  void Clear(int i);
  void Reset(int i);
  static constexpr const int htInitSize = 2;
  static constexpr const int htInitExp = 1;
  static constexpr const double dictForceResizeRatio = 2.0;
  DictType* type_;
  std::vector<std::vector<DictEntry*>> ht_;
  size_t ht_used_[2];
  int ht_size_exp_[2];
  long rehash_idx_;
  int64_t pause_rehash_;
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
  std::function<const unsigned int(const K& key)> hashFunction;
  std::function<K(const K& key)> keyDup;
  std::function<V(const V& val)> valDup;
  std::function<void(K& key)> keyDestructor;
  std::function<void(V& val)> valDestructor;
  std::function<int(const K& key1, const K& key2)> keyCompare;
};

template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Init() {
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>());
  if (!dict->Expand(htInitSize)) {
    return nullptr;
  }
  dict->GetType()->hashFunction = [](const K& key) {
    std::hash<K> h;
    return h(key);
  };
  return dict;
}

template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::Init(
    const typename Dict<K, V>::DictType* type) {
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>());
  if (!dict->Expand(htInitSize)) {
    return nullptr;
  }
  dict->GetType() = type;
  return dict;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::Find(const K& key) {
  for (int i = 0; i < ht_.size(); ++i) {
    int idx = KeyHashIndex(key, i);
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

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::Unlink(const K& key) {
  RehashStep();
  for (int i = 0; i < ht_.size(); ++i) {
    int idx = KeyHashIndex(key, i);
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

template <typename K, typename V>
int Dict<K, V>::Scan(int cursor, dictScanFunc callback) {
  PauseRehashing();
  if (cursor < ht_[0].size()) {
    const DictEntry* de = ht_[0][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de);
      de = next;
    }
  }
  if (IsRehashing() && cursor < ht_[1].size()) {
    const DictEntry* de = ht_[1][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de);
      de = next;
    }
  }
  if (cursor >= std::max(ht_[0].size(), ht_[1].size())) {
    cursor = -1;
  }
  ResumeRehashing();
  return ++cursor;
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
  delete type_;
  type_ = nullptr;
}

template <typename K, typename V>
Dict<K, V>::Dict() : rehash_idx_(-1), pause_rehash_(0), type_(new DictType()) {
  ht_.resize(2);
  Reset(0);
  Reset(1);
}

template <typename K, typename V>
unsigned int Dict<K, V>::HtMask(int i) {
  return ht_size_exp_[i] == -1 ? 0 : (1 << ht_size_exp_[i]) - 1;
}

template <typename K, typename V>
unsigned int Dict<K, V>::KeyHashIndex(const K& key, int i) {
  assert(type_->hashFunction);
  return (type_->hashFunction(key)) & HtMask(i);
}

template <typename K, typename V>
unsigned int Dict<K, V>::NextExp(int val) {
  if (val < 0) return htInitExp;

  int i = 1;
  while ((1 << i) < val) ++i;
  return i;
}

template <typename K, typename V>
bool Dict<K, V>::IsEqual(const K& key1, const K& key2) {
  return key1 == key2 ||
         (type_->keyCompare && !(type_->keyCompare(key1, key2)));
}

template <typename K, typename V>
void Dict<K, V>::SetKey(DictEntry* entry, const K& key) {
  entry->key = type_->keyDup ? type_->keyDup(key) : key;
}

template <typename K, typename V>
void Dict<K, V>::SetVal(DictEntry* entry, const V& val) {
  entry->val = type_->valDup ? type_->valDup(val) : val;
}

template <typename K, typename V>
void Dict<K, V>::FreeKey(DictEntry* entry) {
  if (type_->keyDestructor) {
    type_->keyDestructor(entry->key);
  }
}

template <typename K, typename V>
void Dict<K, V>::FreeVal(DictEntry* entry) {
  if (type_->valDestructor) {
    type_->valDestructor(entry->val);
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
unsigned int Dict<K, V>::KeyIndex(const K& key,
                                  Dict<K, V>::DictEntry** existing) {
  if (existing) {
    *existing = nullptr;
  }
  ExpandIfNeeded();
  int idx = -1;
  for (int i = 0; i < ht_.size(); ++i) {
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

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::AddRaw(
    const K& key, Dict<K, V>::DictEntry** existing) {
  RehashStep();
  int idx = KeyIndex(key, existing);
  if (idx < 0) {
    return nullptr;
  }
  int i = IsRehashing() ? 1 : 0;
  DictEntry* de = new DictEntry();
  SetKey(de, key);
  de->next = ht_[i][idx];
  ht_[i][idx] = de;
  ++ht_used_[i];
  return de;
}

template <typename K, typename V>
void Dict<K, V>::ExpandIfNeeded() {
  if ((double)ht_used_[0] / HtSize(ht_size_exp_[0]) >= dictForceResizeRatio) {
    Expand(ht_used_[0] + 1);
  }
}

template <typename K, typename V>
bool Dict<K, V>::Expand(size_t size) {
  if (IsRehashing() || size < ht_used_[0]) {
    return false;
  }
  int new_exp = NextExp(size);
  if (new_exp <= ht_size_exp_[0]) {
    return false;
  }
  size_t new_size = HtSize(new_exp);
  printf("dict expand to %zu %d\n", new_size, new_exp);
  if (new_size < size ||
      new_size * sizeof(DictEntry*) < size * sizeof(DictEntry*)) {
    return false;
  }
  if (ht_size_exp_[0] < 0) {
    ht_size_exp_[0] = new_exp;
    ht_[0].resize(new_size);
    ht_used_[0] = 0;
    rehash_idx_ = -1;
    return true;
  }
  ht_size_exp_[1] = new_exp;
  ht_[1].resize(new_size);
  ht_used_[1] = 0;
  rehash_idx_ = 0;
  return true;
}

template <typename K, typename V>
void Dict<K, V>::RehashStep() {
  if (pause_rehash_ == 0) {
    Rehash(1);
  }
}

/* Perform n steps of rehashing. Returns true if the rehashing is still not
 * completed and vice versa */
template <typename K, typename V>
bool Dict<K, V>::Rehash(int n) {
  if (!IsRehashing()) {
    return false;
  }
  int empty_visit = n * 10;
  int dict_size = HtSize(ht_size_exp_[0]);
  while (n > 0 && rehash_idx_ < dict_size && ht_used_[0] && empty_visit) {
    DictEntry* de = ht_[0][rehash_idx_];
    if (!de) {
      --empty_visit, ++rehash_idx_;
      continue;
    }
    while (de) {
      int idx = KeyHashIndex(de->key, 1);
      DictEntry* next = de->next;
      de->next = ht_[1][idx];
      ht_[1][idx] = de;
      --ht_used_[0], ++ht_used_[1];
      de = next;
    }
    ht_[0][rehash_idx_] = nullptr;
    --n, ++rehash_idx_;
  }
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

template <typename K, typename V>
void Dict<K, V>::Clear(int i) {
  for (int j = 0; j < ht_[i].size() && ht_used_[i] > 0; ++j) {
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
  if (i >= ht_.size()) return;
  ht_[i].clear();
  ht_used_[i] = 0;
  ht_size_exp_[i] = -1;
}
}  // namespace in_memory
}  // namespace redis_simple
