#pragma once

#include <iostream>

namespace redis_simple {
namespace in_memory {
template <typename Key>
const auto default_compare = [](const Key& k1, const Key& k2) {
  return k1 < k2 ? -1 : (k1 == k2 ? 0 : 1);
};

template <typename Key>
const auto default_dtr = [](const Key& k) {};

template <typename Key, typename Comparator = decltype(default_compare<Key>),
          typename Destructor = decltype(default_dtr<Key>)>
class Skiplist {
 private:
  struct SkiplistLevel;
  class SkiplistNode;

 public:
  struct SkiplistRangeOption {
    int limit, offset;
  };
  struct SkiplistRangeByIndexSpec {
    mutable int min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    const SkiplistRangeOption* option;
  };
  struct SkiplistRangeByKeySpec {
    int min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    const SkiplistRangeOption* option;
  };
  class Iterator;
  static constexpr const int InitSkiplistLevel = 2;
  Skiplist();
  explicit Skiplist(const size_t level);
  explicit Skiplist(const size_t level, const Comparator& compare);
  explicit Skiplist(const size_t level, const Comparator& compare,
                    const Destructor& dtr);
  Iterator begin() const;
  Iterator end() const;
  const Key& insert(const Key& key);
  bool contains(const Key& key);
  bool del(const Key& key);
  bool update(const Key& key, const Key& new_key);
  const Key& getElementByRank(int rank);
  ssize_t getRankofElement(const Key& key);
  std::vector<Key> rangeByIndex(const SkiplistRangeByIndexSpec* spec);
  int rangeByIndexCount(const SkiplistRangeByIndexSpec* spec);
  std::vector<Key> revRangeByIndex(const SkiplistRangeByIndexSpec* spec);
  int revRangeByIndexCount(const SkiplistRangeByIndexSpec* spec);
  std::vector<Key> getElementsGt(const Key& start);
  std::vector<Key> getElementsGte(const Key& start);
  std::vector<Key> getElementsLt(const Key& end);
  std::vector<Key> getElementsLte(const Key& end);
  std::vector<Key> rangeByKey(const Key& start, const Key& end);
  std::vector<Key> revRangeByKey(const Key& start, const Key& end);
  const Key& operator[](size_t i);
  size_t size() { return _size; }
  void clear();
  void print() const;
  ~Skiplist();

 private:
  static constexpr const int MaxSkiplistLevel = 16;
  static constexpr const double SkiplistP = 0.5;
  size_t randomLevel();
  bool lt(const Key& k1, const Key& k2);
  bool lte(const Key& k1, const Key& k2);
  bool gt(const Key& k1, const Key& k2);
  bool gte(const Key& k1, const Key& k2);
  bool eq(const Key& k1, const Key& k2);
  void deleteNode(const Key& key, SkiplistNode* prev[MaxSkiplistLevel]);
  const SkiplistNode* getElement(size_t rank);
  bool rebaseAndValidateRangeIndexSpec(const SkiplistRangeByIndexSpec* spec);
  const SkiplistNode* getMinNodeByRangeIndexSpec(
      const SkiplistRangeByIndexSpec* spec);
  const SkiplistNode* getRevMinNodeByRangeIndexSpec(
      const SkiplistRangeByIndexSpec* spec);
  std::vector<Key> _rangeByIndex(const SkiplistRangeByIndexSpec* spec);
  size_t _rangeByIndexCount(const SkiplistRangeByIndexSpec* spec);
  std::vector<Key> _revRangeByIndex(const SkiplistRangeByIndexSpec* spec);
  size_t _revRangeByIndexCount(const SkiplistRangeByIndexSpec* spec);
  std::vector<Key> getElementsGt(const Key& start, bool eq);
  std::vector<Key> getElementsLt(const Key& end, bool eq);
  const SkiplistNode* getFirstElementGt(const Key& key, bool eq);
  const SkiplistNode* getLastElementLt(const Key& key, bool eq);
  void reset();
  const SkiplistNode* findLast() const;
  SkiplistNode* head;
  const Comparator compare;
  const Destructor dtr;
  size_t level;
  size_t _size;
};

/* SkiplistLevel */
template <typename Key, typename Comparator, typename Destructor>
struct Skiplist<Key, Comparator, Destructor>::SkiplistLevel {
  explicit SkiplistLevel(SkiplistNode* next, size_t span)
      : next(next), span(0){};
  void reset();
  ~SkiplistLevel() { reset(); }
  SkiplistNode* next;
  size_t span;
};

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::SkiplistLevel::reset() {
  next = nullptr;
  span = 0;
}

/* SkiplistNode */
template <typename Key, typename Comparator, typename Destructor>
class Skiplist<Key, Comparator, Destructor>::SkiplistNode {
 public:
  static SkiplistNode* createSkiplistNode(const Key& key, const size_t level,
                                          const Destructor& dtr);
  static SkiplistNode* createSkiplistNode(const size_t level,
                                          const Destructor& dtr);
  const SkiplistNode* getNext(const size_t level) const {
    return levels[level]->next;
  };
  SkiplistNode* getNext(const size_t level) { return levels[level]->next; };
  void setNext(const size_t level, const SkiplistNode* next) {
    levels[level]->next = const_cast<SkiplistNode*>(next);
  };
  size_t getSpan(const size_t level) const { return levels[level]->span; };
  size_t getSpan(const size_t level) { return levels[level]->span; };
  void setSpan(const size_t level, const size_t span) {
    levels[level]->span = span;
  };
  const SkiplistNode* getPrev() const { return prev; };
  SkiplistNode* getPrev() { return prev; };
  void setPrev(const SkiplistNode* prev_node) {
    prev = const_cast<SkiplistNode*>(prev_node);
  };
  void initLevel(const size_t level) {
    levels[level] = new SkiplistLevel(nullptr, 0);
  };
  void reset();
  ~SkiplistNode();
  Key key;

 private:
  SkiplistNode(const Destructor& dtr) : prev(nullptr), dtr(dtr) {
    memset(levels, 0, sizeof levels);
  };
  explicit SkiplistNode(const Key& key, Destructor dtr)
      : key(key), prev(nullptr), dtr(dtr) {
    memset(levels, 0, sizeof levels);
  };
  SkiplistLevel* levels[MaxSkiplistLevel];
  SkiplistNode* prev;
  const Destructor dtr;
};

template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::SkiplistNode::createSkiplistNode(
    const Key& key, const size_t level, const Destructor& dtr) {
  SkiplistNode* n = new SkiplistNode(key, dtr);
  for (int i = 0; i < level; ++i) {
    n->initLevel(i);
  }
  return n;
}

template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::SkiplistNode::createSkiplistNode(
    const size_t level, const Destructor& dtr) {
  SkiplistNode* n = new SkiplistNode(dtr);
  for (int i = 0; i < level; ++i) {
    n->initLevel(i);
  }
  return n;
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::SkiplistNode::reset() {
  for (int i = 0; i < MaxSkiplistLevel; ++i) {
    delete levels[i];
    levels[i] = nullptr;
  }
  prev = nullptr;
}

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::SkiplistNode::~SkiplistNode() {
  reset();
  dtr(key);
}

/* Iterator */
template <typename Key, typename Comparator, typename Destructor>
class Skiplist<Key, Comparator, Destructor>::Iterator {
 public:
  explicit Iterator(const Skiplist* skiplist);
  explicit Iterator(const Skiplist* skiplist, const SkiplistNode* node);
  Iterator(const Iterator& it);
  void seekToFirst();
  void seekToLast();
  Iterator& operator=(const Iterator& it);
  void operator--();
  void operator++();
  bool operator==(const Iterator& it);
  bool operator!=(const Iterator& it);
  const Key& operator*();

 private:
  const SkiplistNode* node;
  const Skiplist* skiplist;
};

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Iterator::Iterator(
    const Skiplist* skiplist)
    : skiplist(skiplist), node(nullptr) {}

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Iterator::Iterator(
    const Skiplist* skiplist, const SkiplistNode* node)
    : skiplist(skiplist), node(node) {}

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Iterator::Iterator(const Iterator& it)
    : skiplist(it.skiplist), node(it.node) {}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Iterator::seekToFirst() {
  node = skiplist->head->getNext(0);
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Iterator::seekToLast() {
  node = skiplist->findLast();
  if (node == skiplist->head) node = nullptr;
}

template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::Iterator&
Skiplist<Key, Comparator, Destructor>::Iterator::operator=(const Iterator& it) {
  skiplist = it.skiplist;
  node = it.node;
  return *this;
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Iterator::operator++() {
  node = node->getNext(0);
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Iterator::operator--() {
  node = node->getPrev();
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Iterator::operator==(
    const Iterator& it) {
  return skiplist == it.skiplist && node == it.node;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Iterator::operator!=(
    const Iterator& it) {
  return !((*this) == it);
}

template <typename Key, typename Comparator, typename Destructor>
const Key& Skiplist<Key, Comparator, Destructor>::Iterator::operator*() {
  return node->key;
}

/* Skiplist */
template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Skiplist()
    : level(InitSkiplistLevel),
      head(SkiplistNode::createSkiplistNode(InitSkiplistLevel,
                                            default_dtr<Key>)),
      compare(default_compare<Key>),
      dtr(default_dtr<Key>),
      _size(0){};

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Skiplist(const size_t level)
    : level(level),
      head(SkiplistNode::createSkiplistNode(level, default_dtr<Key>)),
      compare(default_compare<Key>),
      dtr(default_dtr<Key>),
      _size(0){};

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Skiplist(const size_t level,
                                                const Comparator& compare)
    : level(level),
      head(SkiplistNode::createSkiplistNode(level, default_dtr<Key>)),
      compare(compare),
      dtr(default_dtr<Key>),
      _size(0){};

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Skiplist(const size_t level,
                                                const Comparator& compare,
                                                const Destructor& dtr)
    : level(level),
      head(SkiplistNode::createSkiplistNode(level, dtr)),
      compare(compare),
      dtr(dtr),
      _size(0){};

template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::Iterator
Skiplist<Key, Comparator, Destructor>::begin() const {
  return Iterator(this, head->getNext(0));
}

template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::Iterator
Skiplist<Key, Comparator, Destructor>::end() const {
  return Iterator(this, findLast()->getNext(0));
}

template <typename Key, typename Comparator, typename Destructor>
size_t Skiplist<Key, Comparator, Destructor>::randomLevel() {
  size_t level = 1;
  while (level < MaxSkiplistLevel && ((double)rand() / RAND_MAX) < SkiplistP) {
    ++level;
  }
  return level;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::lt(const Key& k1, const Key& k2) {
  return compare(k1, k2) < 0;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::lte(const Key& k1, const Key& k2) {
  return lt(k1, k2) || eq(k1, k2);
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::gt(const Key& k1, const Key& k2) {
  return compare(k1, k2) > 0;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::gte(const Key& k1, const Key& k2) {
  return gt(k1, k2) || eq(k1, k2);
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::eq(const Key& k1, const Key& k2) {
  return compare(k1, k2) == 0;
}

template <typename Key, typename Comparator, typename Destructor>
const Key& Skiplist<Key, Comparator, Destructor>::insert(const Key& key) {
  int insert_level = randomLevel();

  /*
   * If random level is larger than level
   * init empty skiplist level for extra levels and update level to the random
   * level.
   */
  for (int i = level; i < insert_level; ++i) {
    head->initLevel(i);
    head->setSpan(i, _size);
  }

  if (insert_level > level) level = insert_level;

  const SkiplistNode* update[level];
  size_t rank[level];

  /* get the last key less than current key in each level */
  const SkiplistNode* n = head;
  for (int i = level - 1; i >= 0; --i) {
    rank[i] = (i == level - 1) ? 0 : rank[i + 1];
    while (n->getNext(i) && lt(n->getNext(i)->key, key)) {
      rank[i] += n->getSpan(i);
      n = n->getNext(i);
    }
    if (n->getNext(i) && eq(n->getNext(i)->key, key)) {
      printf("skiplist: update an existing element\n");
      return n->getNext(i)->key;
    }
    update[i] = n;
  }

  SkiplistNode* node = SkiplistNode::createSkiplistNode(key, level, dtr);
  /*
   * insert the key and update span
   */
  for (int i = 0; i < level; ++i) {
    if (i < insert_level) {
      /* need to insert the key */
      size_t span = update[i]->getSpan(i);
      node->setNext(i, update[i]->getNext(i));
      node->setSpan(i, span - rank[0] + rank[i]);
      SkiplistNode* n = const_cast<SkiplistNode*>(update[i]);
      n->setNext(i, node);
      n->setSpan(i, rank[0] - rank[i] + 1);
    } else {
      /* only increase span by 1 */
      SkiplistNode* n = const_cast<SkiplistNode*>(update[i]);
      n->setSpan(i, update[i]->getSpan(i) + 1);
    }
  }
  /* for level 0, update backward pointer since it's a double linked list */
  node->setPrev(update[0]);
  if (node->getNext(0)) {
    node->getNext(0)->setPrev(node);
  }
  ++_size;
  printf("skiplist: new element inserted\n");
  return node->key;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::contains(const Key& key) {
  const SkiplistNode* n = head;
  for (int i = level - 1; i >= 0; --i) {
    while (n->getNext(i) && lt(n->getNext(i)->key, key)) {
      n = n->getNext(i);
    }
    const SkiplistNode* next = n->getNext(i);
    if (next && eq(next->key, key)) {
      return true;
    }
  }
  return false;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::del(const Key& key) {
  SkiplistNode* n = head;
  SkiplistNode* update[MaxSkiplistLevel];
  memset(update, 0, sizeof update);
  bool exist = false;
  for (int i = level - 1; i >= 0; --i) {
    while (n->getNext(i) && lt(n->getNext(i)->key, key)) {
      n = n->getNext(i);
    }
    if (n->getNext(i) && eq(n->getNext(i)->key, key)) {
      exist = true;
    }
    update[i] = n;
  }
  if (!exist) return false;
  deleteNode(key, update);
  return true;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::update(const Key& key,
                                                   const Key& new_key) {
  SkiplistNode* update[MaxSkiplistLevel];
  memset(update, 0, sizeof(update));
  SkiplistNode* node = head;
  for (int i = level - 1; i >= 0; --i) {
    while (node->getNext(i) && lt(node->getNext(i)->key, key)) {
      node = node->getNext(i);
    }
    update[i] = node;
  }
  if (!update[0]->getNext(0) || !eq(update[0]->getNext(0)->key, key)) {
    /* key not found */
    return false;
  }
  /* the first getNext(0) must return a non null value since it's the node
   * containing the original key */
  const SkiplistNode* next_next = update[0]->getNext(0)->getNext(0);
  if ((update[0] == head || gte(new_key, update[0]->key)) &&
      (!next_next || lte(new_key, next_next->key))) {
    /* if in the key's position is not changed, update the key directly */
    SkiplistNode* next = update[0]->getNext(0);
    next->key = new_key;
    /* delete the old key */
    dtr(key);
  } else {
    /* otherwise, delete the original node and insert a new one */
    deleteNode(key, update);
    const Key& k = insert(new_key);
  }
  return true;
}

template <typename Key, typename Comparator, typename Destructor>
const Key& Skiplist<Key, Comparator, Destructor>::getElementByRank(int rank) {
  if (rank < 0) {
    rank += _size;
  }
  if (rank < 0) {
    throw std::out_of_range("skiplist index out of bound");
  }
  const SkiplistNode* node = getElement(rank);
  if (!node) {
    throw std::out_of_range("skiplist index out of bound");
  }
  return node->key;
}

template <typename Key, typename Comparator, typename Destructor>
ssize_t Skiplist<Key, Comparator, Destructor>::getRankofElement(
    const Key& key) {
  size_t rank = 0;
  const SkiplistNode* node = head;
  for (int i = level - 1; i >= 0; --i) {
    while (node->getNext(i) && lt(node->getNext(i)->key, key)) {
      rank += node->getSpan(i);
      node = node->getNext(i);
    }
    if (node->getNext(i) && eq(node->getNext(i)->key, key)) {
      return rank + node->getSpan(i) - 1;
    }
  }
  return -1;
}

template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::rangeByIndex(
    const SkiplistRangeByIndexSpec* spec) {
  return rebaseAndValidateRangeIndexSpec(spec) ? _rangeByIndex(spec)
                                               : std::vector<Key>();
}

template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::revRangeByIndex(
    const SkiplistRangeByIndexSpec* spec) {
  return rebaseAndValidateRangeIndexSpec(spec) ? _revRangeByIndex(spec)
                                               : std::vector<Key>();
}

template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::getElementsGt(
    const Key& start) {
  return getElementsGt(start, false);
}

template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::getElementsGte(
    const Key& start) {
  return getElementsGt(start, true);
}

template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::getElementsLt(
    const Key& end) {
  return getElementsLt(end, false);
}

template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::getElementsLte(
    const Key& end) {
  return getElementsLt(end, true);
}

/*
 * return all keys within the range [start, end) in ascending order
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::rangeByKey(
    const Key& start, const Key& end) {
  if (gte(start, end)) return {};

  const SkiplistNode* ns = getFirstElementGt(start, true);
  std::vector<Key> keys;
  while (ns && lt(ns->key, end)) {
    keys.push_back(ns->key);
    ns = ns->getNext(0);
  }
  return keys;
}

/*
 * return all keys within the range [start, end] in descending order
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::revRangeByKey(
    const Key& start, const Key& end) {
  if (gte(start, end)) return {};

  const SkiplistNode* ns = getLastElementLt(end, false);
  std::vector<Key> keys;
  while (ns && gte(ns->key, start)) {
    keys.push_back(ns->key);
    ns = ns->getPrev();
  }
  return keys;
}

template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::getElementsGt(
    const Key& start, bool eq) {
  const SkiplistNode* ns = getFirstElementGt(start, eq);
  std::vector<Key> keys;
  while (ns) {
    keys.push_back(ns->key);
    ns = ns->getNext(0);
  }
  return keys;
}

template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::getElementsLt(
    const Key& start, bool eq) {
  const SkiplistNode* ns = getLastElementLt(start, eq);
  if (ns == head) {
    return {};
  }
  std::vector<Key> keys;
  const SkiplistNode* n = head->getNext(0);
  while (n != ns->getNext(0)) {
    keys.push_back(n->key);
    n = n->getNext(0);
  }
  return keys;
}

template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::getFirstElementGt(const Key& key,
                                                         bool eq) {
  const SkiplistNode* node = head;
  for (int i = level - 1; i >= 0; --i) {
    while (node->getNext(i) && (eq ? lt(node->getNext(i)->key, key)
                                   : lte(node->getNext(i)->key, key))) {
      node = node->getNext(i);
    }
  }
  return node->getNext(0);
}

template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::getLastElementLt(const Key& key,
                                                        bool eq) {
  const SkiplistNode* node = head;
  for (int i = level - 1; i >= 0; --i) {
    while (node->getNext(i) && (eq ? lte(node->getNext(i)->key, key)
                                   : lt(node->getNext(i)->key, key))) {
      node = node->getNext(i);
    }
  }
  return node;
}

template <typename Key, typename Comparator, typename Destructor>
const Key& Skiplist<Key, Comparator, Destructor>::operator[](size_t i) {
  const SkiplistNode* node = getElement(i);
  if (node == nullptr) {
    throw std::out_of_range("skiplist index out of bound");
  }
  return node->key;
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::clear() {
  reset();
  level = InitSkiplistLevel;
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::print() const {
  const SkiplistNode* node = head;
  for (int i = level - 1; i >= 0; --i) {
    printf("h%d", i);
    while (node) {
      std::cout << node->key;
      printf("---%zu---", node->getSpan(i));
      node = node->getNext(i);
    }
    printf("end\n");
    node = head;
  }
}

/*
 * the function assumes that the node exists in the skiplist.
 * should make sure the node contained in the skiplist before calling this
 * function.
 */
template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::deleteNode(
    const Key& key, SkiplistNode* update[MaxSkiplistLevel]) {
  SkiplistNode* node_to_delete = update[0]->getNext(0);
  for (int i = level - 1; i >= 0; --i) {
    const SkiplistNode* next = update[i]->getNext(i);
    if (next && eq(next->key, key)) {
      update[i]->setNext(i, next ? next->getNext(i) : nullptr);
      update[i]->setSpan(
          i, update[i]->getSpan(i) + (next ? next->getSpan(i) : 0) - 1);
    } else if (update[i]) {
      update[i]->setSpan(i, update[i]->getSpan(i) - 1);
    }
  }
  /* for level 0, update backward pointer since it's a double linked list */
  if (update[0]->getNext(0)) {
    update[0]->getNext(0)->setPrev(update[0]);
  }
  delete node_to_delete;
  --_size;
}

template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::getElement(size_t rank) {
  if (rank >= _size) {
    return nullptr;
  }
  size_t span = 0;
  const SkiplistNode* node = head;
  for (int i = level - 1; i >= 0; --i) {
    while (node->getNext(i) && (span + node->getSpan(i) < rank + 1)) {
      span += node->getSpan(i);
      node = node->getNext(i);
    }

    if (node->getNext(i) && span + node->getSpan(i) == rank + 1) {
      return node->getNext(i);
    }
  }
  return nullptr;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::rebaseAndValidateRangeIndexSpec(
    const SkiplistRangeByIndexSpec* spec) {
  if (spec->min < 0) {
    spec->min += _size;
  }
  if (spec->max < 0) {
    spec->max += _size;
  }
  if (spec->min < 0 || spec->max < 0 ||
      ((spec->minex || spec->maxex) && spec->min >= spec->max) ||
      ((!spec->minex && !spec->maxex) && spec->min > spec->max)) {
    return false;
  }
  return true;
}

template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::getMinNodeByRangeIndexSpec(
    const SkiplistRangeByIndexSpec* spec) {
  const SkiplistNode* node = getElement(spec->min);
  /* if min index is excluded, start at the next node */
  if (node && spec->minex) {
    node = node->getNext(0);
  }
  return node;
}

template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::getRevMinNodeByRangeIndexSpec(
    const SkiplistRangeByIndexSpec* spec) {
  const SkiplistNode* node = getElement(_size - 1 - spec->min);
  /* if min index is excluded, start at the previous node */
  if (node && spec->minex) {
    node = node->getPrev();
  }
  return node;
}

/*
 * Get elements in range by index. The function assumes the spec are valid with
 * non-negative min and max value. Should call rebaseAndValidateRangeIndexSpec
 * before calling this function.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::_rangeByIndex(
    const SkiplistRangeByIndexSpec* spec) {
  const SkiplistNode* node = getMinNodeByRangeIndexSpec(spec);
  if (!node) {
    return {};
  }
  std::vector<Key> keys;
  int i = 0, start = spec->min + (spec->minex ? 1 : 0),
      end = spec->max + (spec->maxex ? -1 : 0);
  const int limit = spec->option ? spec->option->limit : -1,
            offset = spec->option ? spec->option->offset : 0;
  while (node && start <= end) {
    /* return the keys if the number reach the limit, only works when the limit
     * is set to a non-negative value */
    if (limit >= 0 && keys.size() >= limit) {
      return keys;
    }
    const SkiplistNode* n = node;
    node = node->getNext(0);
    ++start;
    /* skip the key if the current index is less than the specified offset */
    if (i++ < offset) {
      continue;
    }
    keys.push_back(n->key);
  }
  return keys;
}

/*
 * Get elements in range by index in count. The function assumes the spec are
 * valid with non-negative min and max value. Should call
 * rebaseAndValidateRangeIndexSpec before calling this function.
 */
template <typename Key, typename Comparator, typename Destructor>
size_t Skiplist<Key, Comparator, Destructor>::_rangeByIndexCount(
    const SkiplistRangeByIndexSpec* spec) {
  const SkiplistNode* node = getMinNodeByRangeIndexSpec(spec);
  if (!node) {
    return {};
  }
  size_t num = 0;
  int i = 0, start = spec->min + (spec->minex ? 1 : 0),
      end = spec->max + (spec->maxex ? -1 : 0);
  const int limit = spec->option ? spec->option->limit : -1,
            offset = spec->option ? spec->option->offset : 0;
  while (node && start <= end) {
    /* return the keys if the number reach the limit, only works when the limit
     * is set to a non-negative value */
    if (limit >= 0 && num >= limit) {
      return num;
    }
    node = node->getNext(0);
    ++start;
    /* skip the key if the current index is less than the specified offset */
    if (i++ < offset) {
      continue;
    }
    ++num;
  }
  return num;
}

/*
 * Get elements in reverse range by index. The function assumes the spec are
 * valid with non-negative min and max value. Should call
 * rebaseAndValidateRangeIndexSpec before calling this function.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::_revRangeByIndex(
    const SkiplistRangeByIndexSpec* spec) {
  const SkiplistNode* node = getRevMinNodeByRangeIndexSpec(spec);
  if (!node) {
    return {};
  }
  std::vector<Key> keys;
  int i = 0, start = spec->min + (spec->minex ? 1 : 0),
      end = spec->max + (spec->maxex ? -1 : 0);
  const int limit = spec->option ? spec->option->limit : -1,
            offset = spec->option ? spec->option->offset : 0;
  while (node && start <= end) {
    /* return the keys if the number reach the limit, only works when the limit
     * is set to a non-negative value */
    if (limit >= 0 && keys.size() >= limit) {
      return keys;
    }
    const SkiplistNode* n = node;
    node = node->getPrev();
    ++start;
    /* skip the key if the current index is less than the specified offset */
    if (i++ < offset) {
      continue;
    }
    keys.push_back(n->key);
  }
  return keys;
}

/*
 * Get elements in range by index in count. The function assumes the spec are
 * valid with non-negative min and max value. Should call
 * rebaseAndValidateRangeIndexSpec before calling this function.
 */
template <typename Key, typename Comparator, typename Destructor>
size_t Skiplist<Key, Comparator, Destructor>::_revRangeByIndexCount(
    const SkiplistRangeByIndexSpec* spec) {
  const SkiplistNode* node = getMinNodeByRangeIndexSpec(spec);
  if (!node) {
    return {};
  }
  size_t num = 0;
  int i = 0, start = spec->min + (spec->minex ? 1 : 0),
      end = spec->max + (spec->maxex ? -1 : 0);
  const int limit = spec->option ? spec->option->limit : -1,
            offset = spec->option ? spec->option->offset : 0;
  while (node && start <= end) {
    /* return the keys if the number reach the limit, only works when the limit
     * is set to a non-negative value */
    if (limit >= 0 && num >= limit) {
      return num;
    }
    node = node->getPrev();
    ++start;
    /* skip the key if the current index is less than the specified offset */
    if (i++ < offset) {
      continue;
    }
    ++num;
  }
  return num;
}

template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::findLast() const {
  const SkiplistNode* node = head;
  int l = level;
  while (--l >= 0) {
    while (node->getNext(l)) {
      node = node->getNext(l);
    }
  }
  return node;
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::reset() {
  SkiplistNode* node = head->getNext(0);
  while (node) {
    SkiplistNode* next = node->getNext(0);
    delete node;
    node = next;
  }
  head->reset();
  _size = 0;
}

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::~Skiplist() {
  reset();
}
}  // namespace in_memory
}  // namespace redis_simple
