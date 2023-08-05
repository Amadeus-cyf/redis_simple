#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace redis_simple {
namespace in_memory {
enum class DictStatus {
  dictOK = 0,
  dictErr = -1,
};

template <typename K, typename V>
class Dict {
 public:
  struct DictEntry;
  struct DictType;
  using dictScanFunc = void (*)(const DictEntry*);
  static std::unique_ptr<Dict<K, V>> init();
  static std::unique_ptr<Dict<K, V>> init(const DictType* type);
  DictType* getType() { return type; }
  size_t size() { return ht_used[0] + ht_used[1]; }
  DictEntry* find(const K& key);
  DictEntry* find(K&& key);
  DictStatus add(const K& key, const V& val);
  DictStatus add(K&& key, V&& val);
  DictEntry* addOrFind(const K& key);
  DictEntry* addOrFind(K&& key, V&& val);
  DictStatus replace(const K& key, const V& val);
  DictStatus replace(K&& key, V&& val);
  DictStatus del(const K& key);
  DictStatus del(K&& key);
  DictEntry* unlink(const K& key);
  DictEntry* unlink(K&& key);
  int scan(int cursor, dictScanFunc callback);
  void clear();
  ~Dict();

 private:
  Dict();
  size_t htSize(int exp) { return exp < 0 ? 0 : 1 << exp; }
  bool isRehashing() { return rehash_idx >= 0; }
  void pauseRehashing() { ++pause_rehash; }
  void resumeRehashing() {
    if (pause_rehash > 0) --pause_rehash;
  }
  unsigned int _htMask(int i);
  unsigned int _keyHashIndex(const K& key, int i);
  unsigned int nextExp(int val);
  bool isEqual(const K& key1, const K& key2);
  void setKey(DictEntry* entry, const K& key);
  void setVal(DictEntry* entry, const V& val);
  void freeKey(DictEntry* entry);
  void freeVal(DictEntry* entry);
  void freeUnlinkedEntry(DictEntry* entry);
  unsigned int keyIndex(const K& key, DictEntry** existing);
  DictEntry* _addRaw(const K& key, DictEntry** existing);
  DictEntry* _unlink(const K& key);
  void _dictExpandIfNeeded();
  DictStatus _dictExpand(size_t size);
  void _dictRehashStep();
  bool _dictRehash(int n);
  void _dictClear(int i);
  void _dictReset(int i);
  static constexpr const int HTInitSize = 2;
  static constexpr const int HTInitExp = 1;
  static constexpr const double DictForceResizeRatio = 2.0;
  DictType* type;
  std::vector<std::vector<DictEntry*>> ht;
  size_t ht_used[2];
  int ht_size_exp[2];
  long rehash_idx;
  int64_t pause_rehash;
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
std::unique_ptr<Dict<K, V>> Dict<K, V>::init() {
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>());
  if (dict->_dictExpand(HTInitSize) == DictStatus::dictErr) {
    return nullptr;
  }
  dict->getType()->hashFunction = [](const K& key) {
    std::hash<K> h;
    return h(key);
  };
  return dict;
}

template <typename K, typename V>
std::unique_ptr<Dict<K, V>> Dict<K, V>::init(
    const typename Dict<K, V>::DictType* type) {
  std::unique_ptr<Dict<K, V>> dict(new Dict<K, V>());
  if (dict->_dictExpand(HTInitSize) == DictStatus::dictErr) {
    return nullptr;
  }
  dict->getType() = type;
  return dict;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::find(const K& key) {
  for (int i = 0; i < ht.size(); ++i) {
    int idx = _keyHashIndex(key, i);
    DictEntry* entry = ht[i][idx];
    while (entry) {
      if (isEqual(key, entry->key)) {
        return entry;
      }
      entry = entry->next;
    }
    if (!isRehashing()) {
      break;
    }
  }
  return nullptr;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::find(K&& key) {
  return find(key);
}

template <typename K, typename V>
DictStatus Dict<K, V>::add(const K& key, const V& val) {
  DictEntry* entry = _addRaw(key, nullptr);
  if (!entry) {
    return DictStatus::dictErr;
  }
  setVal(entry, val);
  return DictStatus::dictOK;
}

template <typename K, typename V>
DictStatus Dict<K, V>::add(K&& key, V&& val) {
  return add(key, val);
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::addOrFind(const K& key) {
  DictEntry* existing;
  DictEntry* new_entry = _addRaw(key, &existing);
  return new_entry ? new_entry : existing;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::addOrFind(K&& key, V&& val) {
  return addOrFind(key, val);
}

template <typename K, typename V>
DictStatus Dict<K, V>::replace(const K& key, const V& val) {
  DictEntry* existing;
  DictEntry* entry = _addRaw(key, &existing);
  if (entry) {
    setVal(entry, val);
  } else {
    DictEntry auxentry = *existing;
    setVal(existing, val);
    freeVal(&auxentry);
  }
  return DictStatus::dictOK;
}

template <typename K, typename V>
DictStatus Dict<K, V>::replace(K&& key, V&& val) {
  return replace(key, val);
}

template <typename K, typename V>
DictStatus Dict<K, V>::del(const K& key) {
  DictEntry* de = _unlink(key);
  DictStatus ds = de ? DictStatus::dictOK : DictStatus::dictErr;
  freeUnlinkedEntry(de);
  return ds;
}

template <typename K, typename V>
DictStatus Dict<K, V>::del(K&& key) {
  return del(key);
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::unlink(const K& key) {
  return _unlink(key);
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::unlink(K&& key) {
  return unlink(key);
}

template <typename K, typename V>
int Dict<K, V>::scan(int cursor, dictScanFunc callback) {
  pauseRehashing();
  if (cursor < ht[0].size()) {
    const DictEntry* de = ht[0][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de);
      de = next;
    }
  }
  if (isRehashing() && cursor < ht[1].size()) {
    const DictEntry* de = ht[1][cursor];
    while (de) {
      const DictEntry* next = de->next;
      callback(de);
      de = next;
    }
  }
  if (cursor >= std::max(ht[0].size(), ht[1].size())) {
    cursor = -1;
  }
  resumeRehashing();
  return ++cursor;
}

template <typename K, typename V>
void Dict<K, V>::clear() {
  _dictClear(0);
  _dictClear(1);
  rehash_idx = -1;
  pause_rehash = 0;
}

template <typename K, typename V>
Dict<K, V>::~Dict() {
  clear();
  delete type;
  type = nullptr;
}

template <typename K, typename V>
Dict<K, V>::Dict() : rehash_idx(-1), pause_rehash(0), type(new DictType()) {
  ht.resize(2);
  _dictReset(0);
  _dictReset(1);
}

template <typename K, typename V>
unsigned int Dict<K, V>::_htMask(int i) {
  return ht_size_exp[i] == -1 ? 0 : (1 << ht_size_exp[i]) - 1;
}

template <typename K, typename V>
unsigned int Dict<K, V>::_keyHashIndex(const K& key, int i) {
  assert(type->hashFunction);
  return (type->hashFunction(key)) & _htMask(i);
}

template <typename K, typename V>
unsigned int Dict<K, V>::nextExp(int val) {
  if (val < 0) return HTInitExp;

  int i = 1;
  while ((1 << i) < val) ++i;
  return i;
}

template <typename K, typename V>
bool Dict<K, V>::isEqual(const K& key1, const K& key2) {
  return key1 == key2 || (type->keyCompare && !(type->keyCompare(key1, key2)));
}

template <typename K, typename V>
void Dict<K, V>::setKey(DictEntry* entry, const K& key) {
  entry->key = type->keyDup ? type->keyDup(key) : key;
}

template <typename K, typename V>
void Dict<K, V>::setVal(DictEntry* entry, const V& val) {
  entry->val = type->valDup ? type->valDup(val) : val;
}

template <typename K, typename V>
void Dict<K, V>::freeKey(DictEntry* entry) {
  if (type->keyDestructor) {
    type->keyDestructor(entry->key);
  }
}

template <typename K, typename V>
void Dict<K, V>::freeVal(DictEntry* entry) {
  if (type->valDestructor) {
    type->valDestructor(entry->val);
  }
}

template <typename K, typename V>
void Dict<K, V>::freeUnlinkedEntry(DictEntry* entry) {
  if (entry) {
    freeKey(entry);
    freeVal(entry);
    delete entry;
    entry = nullptr;
  }
}

template <typename K, typename V>
unsigned int Dict<K, V>::keyIndex(const K& key,
                                  Dict<K, V>::DictEntry** existing) {
  if (existing) {
    *existing = nullptr;
  }
  _dictExpandIfNeeded();
  int idx = -1;
  for (int i = 0; i < ht.size(); ++i) {
    idx = _keyHashIndex(key, i);
    DictEntry* entry = ht[i][idx];
    while (entry) {
      if (isEqual(entry->key, key)) {
        if (existing) {
          *existing = entry;
          return -1;
        }
      }
      entry = entry->next;
    }
    if (!isRehashing()) {
      break;
    }
  }
  return idx;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::_addRaw(
    const K& key, Dict<K, V>::DictEntry** existing) {
  _dictRehashStep();
  int idx = keyIndex(key, existing);
  if (idx < 0) {
    return nullptr;
  }
  int i = isRehashing() ? 1 : 0;
  DictEntry* de = new DictEntry();
  setKey(de, key);
  de->next = ht[i][idx];
  ht[i][idx] = de;
  ++ht_used[i];
  return de;
}

template <typename K, typename V>
typename Dict<K, V>::DictEntry* Dict<K, V>::_unlink(const K& key) {
  _dictRehashStep();
  for (int i = 0; i < ht.size(); ++i) {
    int idx = _keyHashIndex(key, i);
    DictEntry *entry = ht[i][idx], *prev = nullptr;
    while (entry) {
      if (isEqual(key, entry->key)) {
        if (prev) {
          prev->next = entry->next;
        } else {
          ht[i][idx] = entry->next;
        }
        entry->next = nullptr;
        --ht_used[i];
        return entry;
      }
      prev = entry, entry = entry->next;
    }
    if (!isRehashing()) {
      break;
    }
  }
  return nullptr;
}

template <typename K, typename V>
void Dict<K, V>::_dictExpandIfNeeded() {
  if ((double)ht_used[0] / htSize(ht_size_exp[0]) >= DictForceResizeRatio) {
    _dictExpand(ht_used[0] + 1);
  }
}

template <typename K, typename V>
DictStatus Dict<K, V>::_dictExpand(size_t size) {
  if (isRehashing() || size < ht_used[0]) {
    return DictStatus::dictErr;
  }
  int new_exp = nextExp(size);
  if (new_exp <= ht_size_exp[0]) {
    return DictStatus::dictErr;
  }
  size_t new_size = htSize(new_exp);
  printf("dict expand to %zu %d\n", new_size, new_exp);
  if (new_size < size ||
      new_size * sizeof(DictEntry*) < size * sizeof(DictEntry*)) {
    return DictStatus::dictErr;
  }
  if (ht_size_exp[0] < 0) {
    ht_size_exp[0] = new_exp;
    ht[0].resize(new_size);
    ht_used[0] = 0;
    rehash_idx = -1;
    return DictStatus::dictOK;
  }
  ht_size_exp[1] = new_exp;
  ht[1].resize(new_size);
  ht_used[1] = 0;
  rehash_idx = 0;
  return DictStatus::dictOK;
}

template <typename K, typename V>
void Dict<K, V>::_dictRehashStep() {
  if (pause_rehash == 0) {
    _dictRehash(1);
  }
}

/* Perform n steps of rehashing. Returns true if the rehashing is still not
 * completed and vice versa */
template <typename K, typename V>
bool Dict<K, V>::_dictRehash(int n) {
  if (!isRehashing()) {
    return false;
  }
  int empty_visit = n * 10;
  int dict_size = htSize(ht_size_exp[0]);
  while (n > 0 && rehash_idx < dict_size && ht_used[0] && empty_visit) {
    DictEntry* de = ht[0][rehash_idx];
    if (!de) {
      --empty_visit, ++rehash_idx;
      continue;
    }
    while (de) {
      int idx = _keyHashIndex(de->key, 1);
      DictEntry* next = de->next;
      de->next = ht[1][idx];
      ht[1][idx] = de;
      --ht_used[0], ++ht_used[1];
      de = next;
    }
    ht[0][rehash_idx] = nullptr;
    --n, ++rehash_idx;
  }
  if (ht_used[0] == 0) {
    ht[0] = std::move(ht[1]);
    ht_size_exp[0] = ht_size_exp[1];
    ht_used[0] = ht_used[1];
    _dictReset(1);
    rehash_idx = -1;
    return false;
  }
  /* still exists keys to rehash */
  return true;
}

template <typename K, typename V>
void Dict<K, V>::_dictClear(int i) {
  for (int j = 0; j < ht[i].size() && ht_used[i] > 0; ++j) {
    DictEntry* de = ht[i][j];
    while (de) {
      DictEntry* next = de->next;
      de->next = nullptr;
      freeUnlinkedEntry(de);
      de = next;
      --ht_used[i];
    }
    ht[i][j] = nullptr;
  }
  _dictReset(i);
}

template <typename K, typename V>
void Dict<K, V>::_dictReset(int i) {
  if (i >= ht.size()) return;
  ht[i].clear();
  ht_used[i] = 0;
  ht_size_exp[i] = -1;
}
}  // namespace in_memory
}  // namespace redis_simple
