#pragma once

#include <iostream>

namespace redis_simple {
namespace in_memory {
/* default comparator */
template <typename Key>
const auto default_compare = [](const Key& k1, const Key& k2) {
  return k1 < k2 ? -1 : (k1 == k2 ? 0 : 1);
};

/* default destructor */
template <typename Key>
const auto default_dtr = [](const Key& k) {};

template <typename Key, typename Comparator = decltype(default_compare<Key>),
          typename Destructor = decltype(default_dtr<Key>)>
class Skiplist {
 private:
  struct SkiplistLevel;
  class SkiplistNode;

 public:
  struct SkiplistLimitSpec {
    size_t offset;
    ssize_t count;
  };
  struct SkiplistRangeByRankSpec {
    mutable size_t min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    const SkiplistLimitSpec* limit;
  };
  struct SkiplistRangeByKeySpec {
    SkiplistRangeByKeySpec(const Key& min, bool minex, const Key& max,
                           bool maxex, const SkiplistLimitSpec* limit)
        : min(min), max(max), minex(minex), maxex(maxex), limit(limit){};
    const Key min, max;
    /* are min or max exclusive? */
    bool minex, maxex;
    const SkiplistLimitSpec* limit;
  };
  class Iterator;
  static constexpr const int InitSkiplistLevel = 2;
  Skiplist();
  explicit Skiplist(const size_t level);
  explicit Skiplist(const size_t level, const Comparator& compare);
  explicit Skiplist(const size_t level, const Comparator& compare,
                    const Destructor& dtr);
  Iterator Begin() const;
  Iterator End() const;
  const Key& Insert(const Key& key);
  bool Contains(const Key& key);
  bool Delete(const Key& key);
  bool Update(const Key& key, const Key& new_key);
  const Key& FindKeyByRank(int rank);
  size_t FindRankofKey(const Key& key);
  std::vector<Key> RangeByRank(const SkiplistRangeByRankSpec* spec);
  std::vector<Key> RevRangeByRank(const SkiplistRangeByRankSpec* spec);
  std::vector<Key> RangeByKey(const SkiplistRangeByKeySpec* spec);
  std::vector<Key> RevRangeByKey(const SkiplistRangeByKeySpec* spec);
  size_t Count(const SkiplistRangeByKeySpec* spec);
  const Key& operator[](size_t i);
  size_t Size() { return size_; }
  void Clear();
  void Print() const;
  ~Skiplist();

 private:
  static constexpr const int maxSkiplistLevel = 16;
  static constexpr const double skiplistP = 0.5;
  size_t RandomLevel();
  bool Lt(const Key& k1, const Key& k2);
  bool Lte(const Key& k1, const Key& k2);
  bool Gt(const Key& k1, const Key& k2);
  bool Gte(const Key& k1, const Key& k2);
  bool Eq(const Key& k1, const Key& k2);
  void DeleteNode(const Key& key, SkiplistNode* prev[maxSkiplistLevel]);
  const SkiplistNode* FindKey(size_t rank);
  const SkiplistNode* FindMinNodeByRangeRankSpec(
      const SkiplistRangeByRankSpec* spec);
  const SkiplistNode* FindRevMinNodeByRangeRankSpec(
      const SkiplistRangeByRankSpec* spec);
  bool ValidateRangeRankSpec(const SkiplistRangeByRankSpec* spec);
  bool ValidateRangeKeySpec(const SkiplistRangeByKeySpec* spec);
  std::vector<Key> RangeByRankWithValidSpec(
      const SkiplistRangeByRankSpec* spec);
  std::vector<Key> RevRangeByRankWithValidSpec(
      const SkiplistRangeByRankSpec* spec);
  std::vector<Key> RangeByKeyWithValidSpec(const SkiplistRangeByKeySpec* spec);
  std::vector<Key> RevRangeByKeyWithValidSpec(
      const SkiplistRangeByKeySpec* spec);
  size_t CountWithValidSpec(const SkiplistRangeByKeySpec* spec);
  SkiplistNode* FindKeyGreaterOrEqual(const Key& key, SkiplistNode** const prev,
                                      size_t* const rank);
  const SkiplistNode* FindKeyGreaterOrEqual(const Key& key);
  const SkiplistNode* FindKeyGreaterThan(const Key& key);
  const SkiplistNode* FindKeyLessOrEqual(const Key& key);
  const SkiplistNode* FindKeyLessThan(const Key& key);
  const SkiplistNode* FindKeyLess(const Key& key, bool eq);
  const SkiplistNode* FindLast() const;
  void Reset();
  /* skiplist node head */
  SkiplistNode* head_;
  /* key comparator */
  const Comparator compare_;
  /* key destructor */
  const Destructor dtr_;
  /* number of levels */
  size_t level_;
  /* number of nodes */
  size_t size_;
};

/* SkiplistLevel */
template <typename Key, typename Comparator, typename Destructor>
struct Skiplist<Key, Comparator, Destructor>::SkiplistLevel {
  explicit SkiplistLevel(SkiplistNode* next, size_t span)
      : next_(next), span_(0){};
  void Reset();
  ~SkiplistLevel() { Reset(); }
  SkiplistNode* next_;
  size_t span_;
};

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::SkiplistLevel::Reset() {
  next_ = nullptr;
  span_ = 0;
}

/* SkiplistNode */
template <typename Key, typename Comparator, typename Destructor>
class Skiplist<Key, Comparator, Destructor>::SkiplistNode {
 public:
  static SkiplistNode* Create(const Key& key, const size_t level,
                              const Destructor& dtr);
  static SkiplistNode* Create(const size_t level, const Destructor& dtr);
  const SkiplistNode* Next(const size_t level) const {
    return levels_[level]->next_;
  };
  SkiplistNode* Next(const size_t level) { return levels_[level]->next_; };
  void SetNext(const size_t level, const SkiplistNode* next) {
    levels_[level]->next_ = const_cast<SkiplistNode*>(next);
  };
  size_t Span(const size_t level) const { return levels_[level]->span_; };
  size_t Span(const size_t level) { return levels_[level]->span_; };
  void SetSpan(const size_t level, const size_t span) {
    levels_[level]->span_ = span;
  };
  const SkiplistNode* Prev() const { return prev_; };
  SkiplistNode* Prev() { return prev_; };
  void SetPrev(const SkiplistNode* prev) {
    prev_ = const_cast<SkiplistNode*>(prev);
  };
  void InitLevel(const size_t level) {
    levels_[level] = new SkiplistLevel(nullptr, 0);
  };
  void Reset();
  ~SkiplistNode();
  Key key;

 private:
  SkiplistNode(const Destructor& dtr) : prev_(nullptr), dtr_(dtr) {
    memset(levels_, 0, sizeof levels_);
  };
  explicit SkiplistNode(const Key& key, Destructor dtr)
      : key(key), prev_(nullptr), dtr_(dtr) {
    memset(levels_, 0, sizeof levels_);
  };
  SkiplistLevel* levels_[maxSkiplistLevel];
  SkiplistNode* prev_;
  const Destructor dtr_;
};

template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::SkiplistNode::Create(
    const Key& key, const size_t level, const Destructor& dtr) {
  SkiplistNode* n = new SkiplistNode(key, dtr);
  for (int i = 0; i < level; ++i) {
    n->InitLevel(i);
  }
  return n;
}

template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::SkiplistNode::Create(
    const size_t level, const Destructor& dtr) {
  SkiplistNode* n = new SkiplistNode(dtr);
  for (int i = 0; i < level; ++i) {
    n->InitLevel(i);
  }
  return n;
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::SkiplistNode::Reset() {
  for (int i = 0; i < maxSkiplistLevel; ++i) {
    delete levels_[i];
    levels_[i] = nullptr;
  }
  prev_ = nullptr;
}

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::SkiplistNode::~SkiplistNode() {
  Reset();
  dtr_(key);
}

/* Iterator */
template <typename Key, typename Comparator, typename Destructor>
class Skiplist<Key, Comparator, Destructor>::Iterator {
 public:
  explicit Iterator(const Skiplist* skiplist);
  explicit Iterator(const Skiplist* skiplist, const SkiplistNode* node);
  Iterator(const Iterator& it);
  /* Position at the first node in the list */
  void SeekToFirst();
  /* Position at the last node in the list */
  void SeekToLast();
  Iterator& operator=(const Iterator& it);
  /* move to the next node in the list */
  void operator--();
  /* move to the previous node in the list */
  void operator++();
  bool operator==(const Iterator& it);
  bool operator!=(const Iterator& it);
  const Key& operator*();

 private:
  const SkiplistNode* node_;
  const Skiplist* skiplist_;
};

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Iterator::Iterator(
    const Skiplist* skiplist)
    : skiplist_(skiplist), node_(nullptr) {}

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Iterator::Iterator(
    const Skiplist* skiplist, const SkiplistNode* node)
    : skiplist_(skiplist), node_(node) {}

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Iterator::Iterator(const Iterator& it)
    : skiplist_(it.skiplist), node_(it.node) {}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Iterator::SeekToFirst() {
  node_ = skiplist_->head_->Next(0);
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Iterator::SeekToLast() {
  node_ = skiplist_->FindLast();
  if (node_ == skiplist_->head_) node_ = nullptr;
}

template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::Iterator&
Skiplist<Key, Comparator, Destructor>::Iterator::operator=(const Iterator& it) {
  skiplist_ = it.skiplist;
  node_ = it.node;
  return *this;
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Iterator::operator++() {
  node_ = node_->Next(0);
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Iterator::operator--() {
  node_ = node_->Prev();
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Iterator::operator==(
    const Iterator& it) {
  return skiplist_ == it.skiplist_ && node_ == it.node_;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Iterator::operator!=(
    const Iterator& it) {
  return !((*this) == it);
}

template <typename Key, typename Comparator, typename Destructor>
const Key& Skiplist<Key, Comparator, Destructor>::Iterator::operator*() {
  return node_->key;
}

/* Skiplist */
template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Skiplist()
    : level_(InitSkiplistLevel),
      head_(SkiplistNode::Create(InitSkiplistLevel, default_dtr<Key>)),
      compare_(default_compare<Key>),
      dtr_(default_dtr<Key>),
      size_(0){};

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Skiplist(const size_t level)
    : level_(level),
      head_(SkiplistNode::Create(level, default_dtr<Key>)),
      compare_(default_compare<Key>),
      dtr_(default_dtr<Key>),
      size_(0){};

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Skiplist(const size_t level,
                                                const Comparator& compare)
    : level_(level),
      head_(SkiplistNode::Create(level, default_dtr<Key>)),
      compare_(compare),
      dtr_(default_dtr<Key>),
      size_(0){};

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::Skiplist(const size_t level,
                                                const Comparator& compare,
                                                const Destructor& dtr)
    : level_(level),
      head_(SkiplistNode::Create(level, dtr)),
      compare_(compare),
      dtr_(dtr),
      size_(0){};

/*
 * Return an iterator pointing to the first node.
 */
template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::Iterator
Skiplist<Key, Comparator, Destructor>::Begin() const {
  return Iterator(this, head_->Next(0));
}

/*
 * Return an iterator pointing to the last  node.
 */
template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::Iterator
Skiplist<Key, Comparator, Destructor>::End() const {
  return Iterator(this, FindLast()->Next(0));
}

/*
 * Return a randomnized level used for insertion.
 */
template <typename Key, typename Comparator, typename Destructor>
size_t Skiplist<Key, Comparator, Destructor>::RandomLevel() {
  size_t level = 1;
  while (level < maxSkiplistLevel && ((double)rand() / RAND_MAX) < skiplistP) {
    ++level;
  }
  return level;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Lt(const Key& k1, const Key& k2) {
  return compare_(k1, k2) < 0;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Lte(const Key& k1, const Key& k2) {
  return Lt(k1, k2) || Eq(k1, k2);
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Gt(const Key& k1, const Key& k2) {
  return compare_(k1, k2) > 0;
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Gte(const Key& k1, const Key& k2) {
  return Gt(k1, k2) || Eq(k1, k2);
}

template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Eq(const Key& k1, const Key& k2) {
  return compare_(k1, k2) == 0;
}

/*
 * Insert a new key into the skiplist. If the key already exists, return the
 * node containg the key and skip the insertion. Otherwise, create a new node
 * with the given key and insert it into the skiplist. Return the newly created
 * node as the result.
 */
template <typename Key, typename Comparator, typename Destructor>
const Key& Skiplist<Key, Comparator, Destructor>::Insert(const Key& key) {
  int insert_level = RandomLevel();
  /* if the random level is larger than level
   * init empty skiplist level for extra levels and update level to the random
   * level*/
  for (int i = level_; i < insert_level; ++i) {
    head_->InitLevel(i);
    head_->SetSpan(i, size_);
  }
  if (insert_level > level_) {
    level_ = insert_level;
  }
  /* find previous key at each level having the next key greater than or equal
   * to the given key */
  SkiplistNode* prev[level_];
  size_t rank[level_];
  const SkiplistNode* n = FindKeyGreaterOrEqual(key, prev, rank);
  if (n && Eq(n->key, key)) {
    /* key already exists, do not insert */
    return n->key;
  }
  /* insert the key and update span */
  SkiplistNode* node = SkiplistNode::Create(key, level_, dtr_);
  for (int i = 0; i < level_; ++i) {
    if (i < insert_level) {
      /* need to insert the key */
      size_t span = prev[i]->Span(i);
      node->SetNext(i, prev[i]->Next(i));
      node->SetSpan(i, span - rank[0] + rank[i]);
      prev[i]->SetNext(i, node);
      prev[i]->SetSpan(i, rank[0] - rank[i] + 1);
    } else {
      /* only increase span by 1 */
      prev[i]->SetSpan(i, prev[i]->Span(i) + 1);
    }
  }
  /* for level 0, update backward pointer since it's a double linked list */
  node->SetPrev(prev[0]);
  if (node->Next(0)) {
    node->Next(0)->SetPrev(node);
  }
  ++size_;
  printf("skiplist: new Key inserted\n");
  return node->key;
}

/*
 * Return true if the node containing the key is found in the skiplist.
 */
template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Contains(const Key& key) {
  const SkiplistNode* n = FindKeyGreaterOrEqual(key, nullptr, nullptr);
  return n && Eq(n->key, key);
}

/*
 * Delete the given node containing the key from the skiplist. Return true if
 * the key is deleted, and false if the key is not found.
 */
template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Delete(const Key& key) {
  SkiplistNode* prev[level_];
  memset(prev, 0, sizeof prev);
  const SkiplistNode* n = FindKeyGreaterOrEqual(key, prev, nullptr);
  if (!n || !Eq(n->key, key)) {
    return false;
  }
  DeleteNode(key, prev);
  return true;
}

/*
 * Update the key. Return true if the key is updated, and false if the original
 * key is not found.
 */
template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::Update(const Key& key,
                                                   const Key& new_key) {
  SkiplistNode* prev[level_];
  memset(prev, 0, sizeof(prev));
  const SkiplistNode* n = FindKeyGreaterOrEqual(key, prev, nullptr);
  if (!n || !Eq(n->key, key)) {
    /* key not found */
    return false;
  }

  const SkiplistNode* next = n->Next(0);
  if ((prev[0] == head_ || Gte(new_key, prev[0]->key)) &&
      (!next || Lte(new_key, next->key))) {
    /* if in the key's position is not changed, update the key directly */
    SkiplistNode* next = prev[0]->Next(0);
    next->key = new_key;
    /* delete the old key */
    dtr_(key);
  } else {
    /* otherwise, delete the original node and insert a new one */
    DeleteNode(key, prev);
    const Key& k = Insert(new_key);
  }
  return true;
}

/*
 * Return the key at the given index.
 */
template <typename Key, typename Comparator, typename Destructor>
const Key& Skiplist<Key, Comparator, Destructor>::FindKeyByRank(int rank) {
  if (rank < 0) {
    rank += size_;
  }
  if (rank < 0) {
    throw std::out_of_range("skiplist rank out of bound");
  }
  const SkiplistNode* node = FindKey(rank);
  if (!node) {
    throw std::out_of_range("skiplist rank out of bound");
  }
  return node->key;
}

/*
 * Return the index of the key.
 */
template <typename Key, typename Comparator, typename Destructor>
size_t Skiplist<Key, Comparator, Destructor>::FindRankofKey(const Key& key) {
  size_t rank = 0;
  const SkiplistNode* node = head_;
  for (int i = level_ - 1; i >= 0; --i) {
    while (node->Next(i) && Lt(node->Next(i)->key, key)) {
      rank += node->Span(i);
      node = node->Next(i);
    }
    if (node->Next(i) && Eq(node->Next(i)->key, key)) {
      return rank + node->Span(i) - 1;
    }
  }
  return -1;
}

/*
 * Return all keys within the rank range.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::RangeByRank(
    const SkiplistRangeByRankSpec* spec) {
  return ValidateRangeRankSpec(spec) ? RangeByRankWithValidSpec(spec)
                                     : std::vector<Key>();
}

/*
 * Return the keys which have the reversed rank within the range.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::RevRangeByRank(
    const SkiplistRangeByRankSpec* spec) {
  return ValidateRangeRankSpec(spec) ? RevRangeByRankWithValidSpec(spec)
                                     : std::vector<Key>();
}

/*
 * Return all keys within the range in ascending order.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::RangeByKey(
    const SkiplistRangeByKeySpec* spec) {
  return ValidateRangeKeySpec(spec) ? RangeByKeyWithValidSpec(spec)
                                    : std::vector<Key>();
}

/*
 * Return all keys within the range in descending order.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::RevRangeByKey(
    const SkiplistRangeByKeySpec* spec) {
  return ValidateRangeKeySpec(spec) ? RevRangeByKeyWithValidSpec(spec)
                                    : std::vector<Key>();
}

/*
 * Return the count of keys which have the rank within range.
 */
template <typename Key, typename Comparator, typename Destructor>
size_t Skiplist<Key, Comparator, Destructor>::Count(
    const SkiplistRangeByKeySpec* spec) {
  return ValidateRangeKeySpec(spec) ? CountWithValidSpec(spec) : 0;
}

/*
 * Return the first key greater than or equal to the given key.
 */
template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindKeyGreaterOrEqual(const Key& key) {
  const SkiplistNode* n = FindKeyGreaterOrEqual(key, nullptr, nullptr);
  return n;
}

/*
 * Return the first key greater than the given key.
 */
template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindKeyGreaterThan(const Key& key) {
  const SkiplistNode* n = FindKeyGreaterOrEqual(key, nullptr, nullptr);
  return (n && Eq(n->key, key)) ? n->Next(0) : n;
}

/*
 * Return the last key less than or equal to the given key.
 */
template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindKeyLessOrEqual(const Key& key) {
  return FindKeyLess(key, true);
}

/*
 * Return the last key less than the given key.
 */
template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindKeyLessThan(const Key& key) {
  return FindKeyLess(key, false);
}

template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindKeyLess(const Key& key, bool eq) {
  const SkiplistNode* node = head_;
  for (int i = level_ - 1; i >= 0; --i) {
    while (node->Next(i) &&
           (eq ? Lte(node->Next(i)->key, key) : Lt(node->Next(i)->key, key))) {
      node = node->Next(i);
    }
  }
  return node;
}

template <typename Key, typename Comparator, typename Destructor>
const Key& Skiplist<Key, Comparator, Destructor>::operator[](size_t i) {
  const SkiplistNode* node = FindKey(i);
  if (node == nullptr) {
    throw std::out_of_range("skiplist rank out of bound");
  }
  return node->key;
}

/*
 * Remove all nodes from the skiplist.
 */
template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Clear() {
  Reset();
  level_ = InitSkiplistLevel;
}

template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Print() const {
  const SkiplistNode* node = head_;
  for (int i = level_ - 1; i >= 0; --i) {
    printf("h%d", i);
    while (node) {
      std::cout << node->key;
      printf("---%zu---", node->Span(i));
      node = node->Next(i);
    }
    printf("end\n");
    node = head_;
  }
}

/*
 * Find the first key greater than or equal to the given key.
 * If prev is non-null, fills prev[level] with pointer to previous node at
 * "level" for every level in [0...level_-1].
 * If rank is non-null, fills rank[level] with rank of the given key at "level"
 * for every level in [0...level_-1];
 */
template <typename Key, typename Comparator, typename Destructor>
typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindKeyGreaterOrEqual(
    const Key& key, SkiplistNode** const prev, size_t* const rank) {
  SkiplistNode* n = head_;
  for (int i = level_ - 1; i >= 0; --i) {
    if (rank) {
      rank[i] = (i == level_ - 1) ? 0 : rank[i + 1];
    }
    while (n->Next(i) && Lt(n->Next(i)->key, key)) {
      if (rank) {
        rank[i] += n->Span(i);
      }
      n = n->Next(i);
    }
    if (prev) {
      prev[i] = n;
    }
    if (i == 0 && n) {
      return n->Next(0);
    }
  }
  return nullptr;
}

/*
 * Delete the node from the skiplist.
 * The function assumes that the node exists in the skiplist.
 */
template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::DeleteNode(
    const Key& key, SkiplistNode* update[maxSkiplistLevel]) {
  SkiplistNode* node_to_delete = update[0]->Next(0);
  for (int i = level_ - 1; i >= 0; --i) {
    const SkiplistNode* next = update[i]->Next(i);
    if (next && Eq(next->key, key)) {
      update[i]->SetNext(i, next ? next->Next(i) : nullptr);
      update[i]->SetSpan(i,
                         update[i]->Span(i) + (next ? next->Span(i) : 0) - 1);
    } else if (update[i]) {
      update[i]->SetSpan(i, update[i]->Span(i) - 1);
    }
  }
  /* for level 0, update backward pointer since it's a double linked list */
  if (update[0]->Next(0)) {
    update[0]->Next(0)->SetPrev(update[0]);
  }
  delete node_to_delete;
  --size_;
}

/*
 * Return the key at the given index. Nullptr if index is out of range.
 */
template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindKey(size_t rank) {
  if (rank >= size_) {
    return nullptr;
  }
  size_t span = 0;
  const SkiplistNode* node = head_;
  for (int i = level_ - 1; i >= 0; --i) {
    while (node->Next(i) && (span + node->Span(i) < rank + 1)) {
      span += node->Span(i);
      node = node->Next(i);
    }

    if (node->Next(i) && span + node->Span(i) == rank + 1) {
      return node->Next(i);
    }
  }
  return nullptr;
}

/*
 * Return the first node having rank in the rank range. If spec has minex =
 * true, the returned node should be the one next to the node at the min rank.
 */
template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindMinNodeByRangeRankSpec(
    const SkiplistRangeByRankSpec* spec) {
  const SkiplistNode* node = FindKey(spec->min);
  /* if min rank is excluded, start at the next node */
  if (node && spec->minex) {
    node = node->Next(0);
  }
  return node;
}

/*
 * Return the first node having reversed rank in the rank range. If spec has
 * minex = true, the returned node should be the one previous to the node at the
 * min rank.
 */
template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindRevMinNodeByRangeRankSpec(
    const SkiplistRangeByRankSpec* spec) {
  const SkiplistNode* node = FindKey(size_ - 1 - spec->min);
  /* if min rank is excluded, start at the previous node */
  if (node && spec->minex) {
    node = node->Prev();
  }
  return node;
}

/*
 * Validate the range rank spec. Return true if the spec is
 * valid.
 */
template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::ValidateRangeRankSpec(
    const SkiplistRangeByRankSpec* spec) {
  return spec && spec->min >= 0 && spec->max >= 0 &&
         ((!spec->minex && !spec->maxex && spec->min <= spec->max) ||
          spec->min < spec->max);
}

/*
 * Validate the range key spec. Return true if the spec is
 * valid.
 */
template <typename Key, typename Comparator, typename Destructor>
bool Skiplist<Key, Comparator, Destructor>::ValidateRangeKeySpec(
    const SkiplistRangeByKeySpec* spec) {
  return spec && ((!spec->minex && !spec->maxex && Lte(spec->min, spec->max)) ||
                  Lt(spec->min, spec->max));
}

/*
 * Find keys in reverse range of rank. Rank indicates the position of the key in
 * the skiplist.
 * The function assumes the spec are valid. Should call ValidateRangeRankSpec
 * before calling this function.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key>
Skiplist<Key, Comparator, Destructor>::RangeByRankWithValidSpec(
    const SkiplistRangeByRankSpec* spec) {
  const SkiplistNode* node = FindMinNodeByRangeRankSpec(spec);
  if (!node) {
    return {};
  }
  std::vector<Key> keys;
  size_t i = 0, start = spec->min + (spec->minex ? 1 : 0),
         end = spec->max + (spec->maxex ? -1 : 0),
         offset = spec->limit ? spec->limit->offset : 0;
  const ssize_t count = spec->limit ? spec->limit->count : -1;
  while (node && start <= end) {
    /* return the keys if the number reach the limit, only works when the limit
     * is a non-negative value */
    if (count >= 0 && keys.size() >= count) {
      return keys;
    }
    const SkiplistNode* n = node;
    node = node->Next(0);
    ++start;
    /* add the key if the current rank is larger of equal to the specified
     * offset */
    if (i++ >= offset) {
      keys.push_back(n->key);
    }
  }
  return keys;
}

/*
 * Find keys in reverse range of rank. Rank indicates the position of the key in
 * the skiplist.
 * The function assumes the spec are valid with non-negative min and max value.
 * Should call ValidateRangeRankSpec before calling this function.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key>
Skiplist<Key, Comparator, Destructor>::RevRangeByRankWithValidSpec(
    const SkiplistRangeByRankSpec* spec) {
  const SkiplistNode* node = FindRevMinNodeByRangeRankSpec(spec);
  if (!node) {
    return {};
  }
  std::vector<Key> keys;
  size_t i = 0, start = spec->min + (spec->minex ? 1 : 0),
         end = spec->max + (spec->maxex ? -1 : 0),
         offset = spec->limit ? spec->limit->offset : 0;
  const ssize_t count = spec->limit ? spec->limit->count : -1;
  while (node && start <= end) {
    /* return the keys if the number reach the limit, only works when the limit
     * is a non-negative value */
    if (count >= 0 && keys.size() >= count) {
      return keys;
    }
    const SkiplistNode* n = node;
    node = node->Prev();
    ++start;
    /* add the key if the current rank is larger of equal to the specified
     * offset */
    if (i++ >= offset) {
      keys.push_back(n->key);
    }
  }
  return keys;
}

/*
 * Find keys within the given range of keys.
 * The function assumes spec is valid. Should call ValidateRangeKeySpec
 * before calling this function.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key> Skiplist<Key, Comparator, Destructor>::RangeByKeyWithValidSpec(
    const SkiplistRangeByKeySpec* spec) {
  const SkiplistNode* node = FindKeyGreaterOrEqual(spec->min);
  if (!node) {
    return {};
  }
  /* if min exclusive and the node is equal to min, then move to the next node
   */
  if (spec->minex && Eq(node->key, spec->min)) {
    node = node->Next(0);
  }
  size_t i = 0;
  const size_t offset = spec->limit ? spec->limit->offset : 0;
  const ssize_t count = spec->limit ? spec->limit->count : -1;
  std::vector<Key> keys;
  while (node &&
         (spec->maxex ? Lt(node->key, spec->max) : Lte(node->key, spec->max))) {
    /* return the keys if the number reach the limit, only works when the limit
     * is a non-negative value */
    if (count >= 0 && keys.size() >= count) {
      return keys;
    }
    const SkiplistNode* n = node;
    node = node->Next(0);
    /* add the key if the current rank is larger of equal to the specified
     * offset */
    if (i++ >= offset) {
      keys.push_back(n->key);
    }
  }
  return keys;
}

/*
 * Find keys within the given range of keys.
 * The function assumes spec is valid. Should call ValidateRangeKeySpec
 * before calling this function.
 */
template <typename Key, typename Comparator, typename Destructor>
std::vector<Key>
Skiplist<Key, Comparator, Destructor>::RevRangeByKeyWithValidSpec(
    const SkiplistRangeByKeySpec* spec) {
  const SkiplistNode* node = FindKeyLessOrEqual(spec->max);
  if (!node) {
    return {};
  }
  /* if max exclusive and the node is equal to max, then move to the previous
   * node */
  if (spec->maxex && Eq(node->key, spec->max)) {
    node = node->Prev();
  }
  size_t i = 0;
  const size_t offset = spec->limit ? spec->limit->offset : 0;
  const ssize_t count = spec->limit ? spec->limit->count : -1;
  std::vector<Key> keys;
  while (node != head_ &&
         (spec->minex ? Gt(node->key, spec->min) : Gte(node->key, spec->min))) {
    /* return the keys if the number reach the limit, only works when the limit
     * is a non-negative value */
    if (count >= 0 && keys.size() >= count) {
      return keys;
    }
    const SkiplistNode* n = node;
    node = node->Prev();
    /* add the key if the current rank is larger of equal to the specified
     * offset */
    if (i++ >= offset) {
      keys.push_back(n->key);
    }
  }
  return keys;
}

/*
 * Find number of keys within the given range of keys.
 * The function assumes spec is valid. Should call ValidateRangeKeySpec
 * before calling this function.
 * The result is not related to SkiplistRangeByKeySpec.SkiplistLimitSpec.
 */
template <typename Key, typename Comparator, typename Destructor>
size_t Skiplist<Key, Comparator, Destructor>::CountWithValidSpec(
    const SkiplistRangeByKeySpec* spec) {
  const SkiplistNode* node = FindKeyGreaterOrEqual(spec->min);
  if (!node) {
    return 0;
  }
  /* if min exclusive and the node is equal to min, then move to the next node.
   */
  if (spec->minex && Eq(node->key, spec->min)) {
    node = node->Next(0);
  }
  size_t num = 0;
  std::vector<Key> keys;
  while (node &&
         (spec->maxex ? Lt(node->key, spec->max) : Lte(node->key, spec->max))) {
    node = node->Next(0);
    ++num;
  }
  return num;
}

/*
 * Seek to the last Key of the skiplist
 */
template <typename Key, typename Comparator, typename Destructor>
const typename Skiplist<Key, Comparator, Destructor>::SkiplistNode*
Skiplist<Key, Comparator, Destructor>::FindLast() const {
  const SkiplistNode* node = head_;
  int l = level_;
  while (--l >= 0) {
    while (node->Next(l)) {
      node = node->Next(l);
    }
  }
  return node;
}

/*
 * Reset the skiplist to the initial state.
 */
template <typename Key, typename Comparator, typename Destructor>
void Skiplist<Key, Comparator, Destructor>::Reset() {
  SkiplistNode* node = head_->Next(0);
  while (node) {
    SkiplistNode* next = node->Next(0);
    delete node;
    node = next;
  }
  head_->Reset();
  size_ = 0;
}

template <typename Key, typename Comparator, typename Destructor>
Skiplist<Key, Comparator, Destructor>::~Skiplist() {
  Reset();
}
}  // namespace in_memory
}  // namespace redis_simple
