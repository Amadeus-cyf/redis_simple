#pragma once

#include <iostream>

namespace skiplist {

template <typename Key>
const auto default_compare = [](const Key& k1, const Key& k2) {
  return k1 < k2 ? -1 : (k1 == k2 ? 0 : 1);
};

template <typename Key, typename Comparator = decltype(default_compare<Key>)>
class Skiplist {
 private:
  struct SkiplistLevel;
  struct SkiplistNode;

 public:
  class Iterator {
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
  Skiplist();
  explicit Skiplist(size_t level);
  explicit Skiplist(size_t level, Comparator compare);
  Iterator begin() const;
  Iterator end() const;
  bool insert(const Key& key);
  bool contains(const Key& key);
  bool del(const Key& key);
  bool update(const Key& key, const Key& new_key);
  const Key& getElementByRank(int rank);
  ssize_t getRankofElement(const Key& key);
  std::vector<Key> getElementsByRange(int idx, size_t len);
  std::vector<Key> getElementsByRevRange(int idx, size_t len);
  std::vector<Key> getElementsGt(const Key& start);
  std::vector<Key> getElementsGte(const Key& start);
  std::vector<Key> getElementsLt(const Key& end);
  std::vector<Key> getElementsLte(const Key& end);
  std::vector<Key> getElementsInRange(const Key& start, const Key& end);
  const Key& operator[](size_t i);
  size_t size() { return _size; }
  void clear();
  void print() const;
  ~Skiplist();

 private:
  static constexpr const int InitSkiplistLevel = 2;
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
  std::vector<Key> getElements(size_t rank, size_t len);
  std::vector<Key> getElementsRev(size_t rank, size_t len);
  std::vector<Key> getElementsGt(const Key& start, bool eq);
  std::vector<Key> getElementsLt(const Key& end, bool eq);
  const SkiplistNode* getFirstElementGt(const Key& key, bool eq);
  const SkiplistNode* getLastElementLt(const Key& key, bool eq);
  void reset();
  const SkiplistNode* findLast() const;
  SkiplistNode* head;
  const Comparator compare;
  size_t level;
  size_t _size;
};

/* SkiplistLevel */
template <typename Key, typename Comparator>
struct Skiplist<Key, Comparator>::SkiplistLevel {
  explicit SkiplistLevel(SkiplistNode* next, size_t span)
      : next(next), span(0){};
  void reset();
  ~SkiplistLevel() { reset(); }
  SkiplistNode* next;
  size_t span;
};

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::SkiplistLevel::reset() {
  next = nullptr;
  span = 0;
}

/* SkiplistNode */
template <typename Key, typename Comparator>
struct Skiplist<Key, Comparator>::SkiplistNode {
 public:
  static SkiplistNode* createSkiplistNode(const Key& key, size_t level);
  static SkiplistNode* createSkiplistNode(size_t level);
  const SkiplistNode* getNext(size_t level) const {
    return levels[level]->next;
  };
  SkiplistNode* getNext(size_t level) { return levels[level]->next; };
  void setNext(size_t level, const SkiplistNode* next) {
    levels[level]->next = const_cast<SkiplistNode*>(next);
  };
  size_t getSpan(size_t level) const { return levels[level]->span; };
  size_t getSpan(size_t level) { return levels[level]->span; };
  void setSpan(size_t level, size_t span) { levels[level]->span = span; };
  const SkiplistNode* getPrev() const { return prev; };
  SkiplistNode* getPrev() { return prev; };
  void setPrev(const SkiplistNode* prev_node) {
    prev = const_cast<SkiplistNode*>(prev_node);
  };
  void initLevel(size_t level) {
    levels[level] = new SkiplistLevel(nullptr, 0);
  };
  void reset();
  ~SkiplistNode();
  Key key;

 private:
  SkiplistNode() : prev(nullptr) { memset(levels, 0, sizeof levels); };
  explicit SkiplistNode(const Key& key) : key(key), prev(nullptr) {
    memset(levels, 0, sizeof levels);
  };
  SkiplistLevel* levels[MaxSkiplistLevel];
  SkiplistNode* prev;
};

template <typename Key, typename Comparator>
typename Skiplist<Key, Comparator>::SkiplistNode*
Skiplist<Key, Comparator>::SkiplistNode::createSkiplistNode(const Key& key,
                                                            size_t level) {
  SkiplistNode* n = new SkiplistNode(key);
  for (int i = 0; i < level; ++i) {
    n->initLevel(i);
  }
  return n;
}

template <typename Key, typename Comparator>
typename Skiplist<Key, Comparator>::SkiplistNode*
Skiplist<Key, Comparator>::SkiplistNode::createSkiplistNode(size_t level) {
  SkiplistNode* n = new SkiplistNode();
  for (int i = 0; i < level; ++i) {
    n->initLevel(i);
  }
  return n;
}

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::SkiplistNode::reset() {
  for (int i = 0; i < MaxSkiplistLevel; ++i) {
    delete levels[i];
    levels[i] = nullptr;
  }
  prev = nullptr;
}

template <typename Key, typename Comparator>
Skiplist<Key, Comparator>::SkiplistNode::~SkiplistNode() {
  reset();
}

/* Iterator */
template <typename Key, typename Comparator>
Skiplist<Key, Comparator>::Iterator::Iterator(const Skiplist* skiplist)
    : skiplist(skiplist), node(nullptr) {}

template <typename Key, typename Comparator>
Skiplist<Key, Comparator>::Iterator::Iterator(const Skiplist* skiplist,
                                              const SkiplistNode* node)
    : skiplist(skiplist), node(node) {}

template <typename Key, typename Comparator>
Skiplist<Key, Comparator>::Iterator::Iterator(const Iterator& it)
    : skiplist(it.skiplist), node(it.node) {}

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::Iterator::seekToFirst() {
  node = skiplist->head->getNext(0);
}

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::Iterator::seekToLast() {
  node = skiplist->findLast();
  if (node == skiplist->head) node = nullptr;
}

template <typename Key, typename Comparator>
typename Skiplist<Key, Comparator>::Iterator&
Skiplist<Key, Comparator>::Iterator::operator=(const Iterator& it) {
  skiplist = it.skiplist;
  node = it.node;
  return *this;
}

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::Iterator::operator++() {
  node = node->getNext(0);
}

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::Iterator::operator--() {
  node = node->getPrev();
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::Iterator::operator==(const Iterator& it) {
  return skiplist == it.skiplist && node == it.node;
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::Iterator::operator!=(const Iterator& it) {
  return !((*this) == it);
}

template <typename Key, typename Comparator>
const Key& Skiplist<Key, Comparator>::Iterator::operator*() {
  return node->key;
}

/* Skiplist */
template <typename Key, typename Comparator>
Skiplist<Key, Comparator>::Skiplist()
    : level(InitSkiplistLevel),
      head(SkiplistNode::createSkiplistNode(InitSkiplistLevel)),
      compare(default_compare<Key>),
      _size(0){};

template <typename Key, typename Comparator>
Skiplist<Key, Comparator>::Skiplist(size_t level)
    : level(level),
      head(SkiplistNode::createSkiplistNode(level)),
      compare(default_compare<Key>),
      _size(0){};

template <typename Key, typename Comparator>
Skiplist<Key, Comparator>::Skiplist(size_t level, Comparator compare)
    : level(level),
      head(SkiplistNode::createSkiplistNode(level)),
      compare(compare),
      _size(0){};

template <typename Key, typename Comparator>
typename Skiplist<Key, Comparator>::Iterator Skiplist<Key, Comparator>::begin()
    const {
  return Iterator(this, head->getNext(0));
}

template <typename Key, typename Comparator>
typename Skiplist<Key, Comparator>::Iterator Skiplist<Key, Comparator>::end()
    const {
  return Iterator(this, findLast()->getNext(0));
}

template <typename Key, typename Comparator>
size_t Skiplist<Key, Comparator>::randomLevel() {
  size_t level = 1;
  while (level < MaxSkiplistLevel && ((double)rand() / RAND_MAX) < SkiplistP) {
    ++level;
  }
  return level;
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::lt(const Key& k1, const Key& k2) {
  return compare(k1, k2) < 0;
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::lte(const Key& k1, const Key& k2) {
  return lt(k1, k2) || eq(k1, k2);
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::gt(const Key& k1, const Key& k2) {
  return compare(k1, k2) > 0;
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::gte(const Key& k1, const Key& k2) {
  return gt(k1, k2) || eq(k1, k2);
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::eq(const Key& k1, const Key& k2) {
  return compare(k1, k2) == 0;
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::insert(const Key& key) {
  int insert_level = randomLevel();

  /*
   * If random level is larger than level
   * init empty skiplist level for extra levels and update level to the random
   * level.
   */
  for (int i = level; i < insert_level; ++i) {
    head->initLevel(i);
  }

  if (insert_level > level) level = insert_level;

  const SkiplistNode* update[level];
  size_t rank[level];

  /* get the last key less than current key in each level */
  const SkiplistNode* n = head;
  for (int i = level - 1; i >= 0; --i) {
    rank[i] = (i == level - 1) ? 0 : rank[i + 1];
    while (n->getNext(i) && lte(n->getNext(i)->key, key)) {
      rank[i] += n->getSpan(i);
      n = n->getNext(i);
    }
    if (eq(n->key, key)) return false;
    update[i] = n;
  }

  SkiplistNode* node = SkiplistNode::createSkiplistNode(key, level);

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
  return true;
}

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::contains(const Key& key) {
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

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::del(const Key& key) {
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

template <typename Key, typename Comparator>
bool Skiplist<Key, Comparator>::update(const Key& key, const Key& new_key) {
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
    return true;
  } else {
    /* otherwise, delete the original node and insert a new one */
    deleteNode(key, update);
    return insert(new_key);
  }
}

template <typename Key, typename Comparator>
const Key& Skiplist<Key, Comparator>::getElementByRank(int rank) {
  if (rank < 0) {
    rank += _size;
  }

  if (rank < 0) {
    throw std::out_of_range("skiplist index out of bound");
  }

  const SkiplistNode* node = getElement(rank);

  if (!node) throw std::out_of_range("skiplist index out of bound");

  return node->key;
}

template <typename Key, typename Comparator>
ssize_t Skiplist<Key, Comparator>::getRankofElement(const Key& key) {
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

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsByRange(int idx,
                                                               size_t len) {
  if (idx < 0) {
    idx += _size;
  }
  if (idx < 0) return {};
  return getElements(idx, len);
}

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsByRevRange(int idx,
                                                                  size_t len) {
  if (idx < 0) {
    idx += _size;
  }
  if (idx < 0) return {};
  return getElementsRev(idx, len);
}

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsGt(const Key& start) {
  return getElementsGt(start, false);
}

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsGte(const Key& start) {
  return getElementsGt(start, true);
}

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsLt(const Key& end) {
  return getElementsLt(end, false);
}

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsLte(const Key& end) {
  return getElementsLt(end, true);
}

/*
 * return all keys within the range [start, end)
 */
template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsInRange(const Key& start,
                                                               const Key& end) {
  if (gte(start, end)) return {};

  const SkiplistNode* ns = getFirstElementGt(start, true);

  std::vector<Key> keys;
  while (ns && lt(ns->key, end)) {
    keys.push_back(ns->key);
    ns = ns->getNext(0);
  }

  return keys;
}

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsGt(const Key& start,
                                                          bool eq) {
  const SkiplistNode* ns = getFirstElementGt(start, eq);

  std::vector<Key> keys;
  while (ns) {
    keys.push_back(ns->key);
    ns = ns->getNext(0);
  }

  return keys;
}

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsLt(const Key& start,
                                                          bool eq) {
  const SkiplistNode* ns = getLastElementLt(start, eq);
  if (ns == head) return {};

  std::vector<Key> keys;
  const SkiplistNode* n = head->getNext(0);
  while (n != ns->getNext(0)) {
    keys.push_back(n->key);
    n = n->getNext(0);
  }

  return keys;
}

template <typename Key, typename Comparator>
const typename Skiplist<Key, Comparator>::SkiplistNode*
Skiplist<Key, Comparator>::getFirstElementGt(const Key& key, bool eq) {
  const SkiplistNode* node = head;
  for (int i = level - 1; i >= 0; --i) {
    while (node->getNext(i) && (eq ? lt(node->getNext(i)->key, key)
                                   : lte(node->getNext(i)->key, key))) {
      node = node->getNext(i);
    }
  }
  return node->getNext(0);
}

template <typename Key, typename Comparator>
const typename Skiplist<Key, Comparator>::SkiplistNode*
Skiplist<Key, Comparator>::getLastElementLt(const Key& key, bool eq) {
  const SkiplistNode* node = head;
  for (int i = level - 1; i >= 0; --i) {
    while (node->getNext(i) && (eq ? lte(node->getNext(i)->key, key)
                                   : lt(node->getNext(i)->key, key))) {
      node = node->getNext(i);
    }
  }
  return node;
}

template <typename Key, typename Comparator>
const Key& Skiplist<Key, Comparator>::operator[](size_t i) {
  const SkiplistNode* node = getElement(i);

  if (node == nullptr) throw std::out_of_range("skiplist index out of bound");

  return node->key;
}

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::clear() {
  reset();
  level = InitSkiplistLevel;
}

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::print() const {
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
template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::deleteNode(
    const Key& key, SkiplistNode* update[MaxSkiplistLevel]) {
  SkiplistNode* node_to_delete = update[0]->getNext(0);

  for (int i = level - 1; i >= 0; --i) {
    const SkiplistNode* next = update[i]->getNext(i);
    if (update[i] && next && eq(next->key, key)) {
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

template <typename Key, typename Comparator>
const typename Skiplist<Key, Comparator>::SkiplistNode*
Skiplist<Key, Comparator>::getElement(size_t rank) {
  if (rank >= _size) return nullptr;

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

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElements(size_t rank,
                                                        size_t len) {
  if (rank >= _size || len == 0) return {};

  const SkiplistNode* node = getElement(rank);
  if (node == nullptr) return {};

  std::vector<Key> keys;
  while (node && keys.size() < len) {
    keys.push_back(node->key);
    node = node->getNext(0);
  }

  return keys;
}

template <typename Key, typename Comparator>
std::vector<Key> Skiplist<Key, Comparator>::getElementsRev(size_t rank,
                                                           size_t len) {
  if (rank >= _size || len == 0) return {};

  const SkiplistNode* node = getElement(rank);
  if (node == nullptr) return {};

  std::vector<Key> keys;
  while (node != head && keys.size() < len) {
    keys.push_back(node->key);
    node = node->getPrev();
  }

  return keys;
}

template <typename Key, typename Comparator>
const typename Skiplist<Key, Comparator>::SkiplistNode*
Skiplist<Key, Comparator>::findLast() const {
  const SkiplistNode* node = head;

  int l = level;
  while (--l >= 0) {
    while (node->getNext(l)) {
      node = node->getNext(l);
    }
  }

  return node;
}

template <typename Key, typename Comparator>
void Skiplist<Key, Comparator>::reset() {
  SkiplistNode* node = head->getNext(0);
  while (node) {
    SkiplistNode* next = node->getNext(0);
    delete node;
    node = next;
  }
  head->reset();
  _size = 0;
}

template <typename Key, typename Comparator>
Skiplist<Key, Comparator>::~Skiplist() {
  reset();
}

}  // namespace skiplist
