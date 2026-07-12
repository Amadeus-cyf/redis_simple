#include "memory/quicklist.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace redis_simple::in_memory {
namespace {
constexpr size_t kEntryOverheadEstimate = 8;

bool HasRemoveLimit(size_t limit) { return limit != 0; }

bool ListPackEntryEquals(const ListPack* const listpack, size_t index,
                         std::string_view value) {
  size_t len = 0;
  const auto* data = listpack->Get(index, &len);
  return data != nullptr &&
         std::string_view(reinterpret_cast<const char*>(data), len) == value;
}

ssize_t NextAfterDelete(const ListPack* const listpack, size_t deleted_index) {
  return deleted_index < listpack->TotalBytes() - 1
             ? static_cast<ssize_t>(deleted_index)
             : -1;
}
}  // namespace

QuickList::Node::Node()
    : listpack(std::make_unique<ListPack>()), next(nullptr), prev(nullptr) {}

QuickList::QuickList() : QuickList(kDefaultNodeMaxBytes) {}

QuickList::QuickList(size_t node_max_bytes)
    : head_(nullptr),
      tail_(nullptr),
      size_(0),
      node_count_(0),
      node_max_bytes_(
          std::max(node_max_bytes,
                   static_cast<size_t>(ListPack::kListPackHeaderSize + 1))) {}

bool QuickList::LPush(std::string_view value) {
  if (!head_ || !CanAppendToNode(head_.get(), value)) {
    PrependNode();
  }
  if (!PushToHeadNode(value)) {
    return false;
  }
  ++size_;
  return true;
}

bool QuickList::RPush(std::string_view value) {
  if ((tail_ == nullptr) || !CanAppendToNode(tail_, value)) {
    AppendNode();
  }
  if (!PushToTailNode(value)) {
    return false;
  }
  ++size_;
  return true;
}

std::optional<std::string> QuickList::LPop() {
  if (!head_) {
    return std::nullopt;
  }

  const ssize_t idx = head_->listpack->First();
  auto value = head_->listpack->Get(idx);
  head_->listpack->Delete(idx);
  --size_;
  if (head_->listpack->Size() == 0) {
    DeleteNode(head_.get());
    MergeNext(head_.get());
  } else {
    MergeNext(head_.get());
  }
  return value;
}

std::optional<std::string> QuickList::RPop() {
  if (tail_ == nullptr) {
    return std::nullopt;
  }

  Node* node = tail_;
  const ssize_t idx = node->listpack->Last();
  auto value = node->listpack->Get(idx);
  node->listpack->Delete(idx);
  --size_;
  if (node->listpack->Size() == 0) {
    DeleteNode(node);
    MergeNext(tail_ != nullptr ? tail_->prev : nullptr);
  } else if (node->prev != nullptr) {
    MergeNext(node->prev);
  }
  return value;
}

std::vector<std::string> QuickList::Range(size_t start, size_t stop) const {
  std::vector<std::string> values;
  if (start > stop || start >= size_) {
    return values;
  }
  stop = std::min(stop, size_ - 1);
  values.reserve(stop - start + 1);
  ForEach(start, stop, [&values](std::string_view value) {
    values.emplace_back(value);
    return true;
  });
  return values;
}

bool QuickList::Set(size_t index, std::string_view value) {
  const auto location = Locate(index);
  if (!location.has_value()) {
    return false;
  }
  const auto listpack_index =
      location->node->listpack->IndexAt(location->local_index);
  if (!listpack_index.has_value() ||
      !location->node->listpack->Replace(*listpack_index, value)) {
    return false;
  }
  if (!NormalizeNodeSizesFrom(location->node)) {
    return false;
  }
  MergeAll();
  return true;
}

std::optional<size_t> QuickList::Remove(std::string_view value, size_t limit,
                                        RemoveDirection direction) {
  if (size_ == 0) {
    return 0;
  }

  size_t removed = 0;
  if (direction == RemoveDirection::kFromTail && HasRemoveLimit(limit)) {
    for (Node* node = tail_; node != nullptr && removed < limit;) {
      Node* prev = node->prev;
      ssize_t idx = node->listpack->Last();
      while (idx != -1 && removed < limit) {
        const auto current_index = static_cast<size_t>(idx);
        idx = node->listpack->Prev(current_index);
        if (ListPackEntryEquals(node->listpack.get(), current_index, value)) {
          node->listpack->Delete(current_index);
          --size_;
          ++removed;
        }
      }
      if (node->listpack->Size() == 0) {
        DeleteNode(node);
      }
      node = prev;
    }
    MergeAll();
    return removed;
  }

  for (Node* node = head_.get();
       node != nullptr && (!HasRemoveLimit(limit) || removed < limit);) {
    Node* next = node->next.get();
    ssize_t idx = node->listpack->First();
    while (idx != -1 && (!HasRemoveLimit(limit) || removed < limit)) {
      const auto current_index = static_cast<size_t>(idx);
      if (ListPackEntryEquals(node->listpack.get(), current_index, value)) {
        node->listpack->Delete(current_index);
        --size_;
        ++removed;
        idx = NextAfterDelete(node->listpack.get(), current_index);
        continue;
      }
      idx = node->listpack->Next(current_index);
    }
    if (node->listpack->Size() == 0) {
      DeleteNode(node);
    }
    node = next;
  }
  MergeAll();
  return removed;
}

bool QuickList::Trim(size_t start, size_t stop) {
  if (size_ == 0 || start > stop || start >= size_) {
    Clear();
    return true;
  }

  stop = std::min(stop, size_ - 1);
  const size_t suffix_count = size_ - stop - 1;
  for (size_t i = 0; i < suffix_count; ++i) {
    Node* node = tail_;
    if (node == nullptr) {
      return false;
    }
    const ssize_t idx = node->listpack->Last();
    if (idx == -1) {
      return false;
    }
    node->listpack->Delete(static_cast<size_t>(idx));
    --size_;
    if (node->listpack->Size() == 0) {
      DeleteNode(node);
    }
  }
  for (size_t i = 0; i < start; ++i) {
    Node* node = head_.get();
    if (node == nullptr) {
      return false;
    }
    const ssize_t idx = node->listpack->First();
    if (idx == -1) {
      return false;
    }
    node->listpack->Delete(static_cast<size_t>(idx));
    --size_;
    if (node->listpack->Size() == 0) {
      DeleteNode(node);
    }
  }
  MergeAll();
  return true;
}

std::optional<QuickList::EntryLocation> QuickList::Locate(size_t index) {
  if (index >= size_) {
    return std::nullopt;
  }

  size_t first_index = 0;
  for (Node* node = head_.get(); node != nullptr; node = node->next.get()) {
    const size_t node_size = node->listpack->Size();
    if (index < first_index + node_size) {
      return EntryLocation{node, index - first_index};
    }
    first_index += node_size;
  }
  return std::nullopt;
}

bool QuickList::PushToHeadNode(std::string_view value) {
  return head_ && head_->listpack->Prepend(value);
}

bool QuickList::PushToTailNode(std::string_view value) {
  return (tail_ != nullptr) && tail_->listpack->Append(value);
}

bool QuickList::CanAppendToNode(const Node* node,
                                std::string_view value) const {
  if (node == nullptr) {
    return false;
  }
  const size_t estimated_bytes = value.size() + kEntryOverheadEstimate;
  return node->listpack->Size() == 0 ||
         node->listpack->TotalBytes() + estimated_bytes <= node_max_bytes_;
}

bool QuickList::CanMergeNodes(const Node* left, const Node* right) const {
  if (left == nullptr || right == nullptr) {
    return false;
  }
  const size_t merged_bytes = left->listpack->TotalBytes() +
                              right->listpack->TotalBytes() -
                              ListPack::kListPackHeaderSize;
  return merged_bytes <= node_max_bytes_;
}

bool QuickList::NormalizeNodeSizesFrom(Node* node) {
  for (Node* current = node; current != nullptr;
       current = current->next.get()) {
    while (current->listpack->TotalBytes() > node_max_bytes_ &&
           current->listpack->Size() > 1) {
      if (SplitNode(current) == nullptr) {
        return false;
      }
    }
  }
  return true;
}

QuickList::Node* QuickList::SplitNode(Node* node) {
  if (node == nullptr || node->listpack->Size() <= 1) {
    return nullptr;
  }

  const size_t node_size = node->listpack->Size();
  const size_t split_index = node_size / 2;

  auto right = std::make_unique<Node>();
  if (!node->listpack->ForEach(split_index, node_size - 1,
                               [&right](std::string_view value) {
                                 return right->listpack->Append(value);
                               })) {
    return nullptr;
  }

  for (size_t i = split_index; i < node_size; ++i) {
    const auto idx = node->listpack->IndexAt(split_index);
    if (!idx.has_value()) {
      return nullptr;
    }
    node->listpack->Delete(*idx);
  }
  return InsertNodeAfter(node, std::move(right));
}

QuickList::Node* QuickList::InsertNodeAfter(Node* node,
                                            std::unique_ptr<Node> new_node) {
  if (node == nullptr || new_node == nullptr) {
    return nullptr;
  }

  Node* new_node_ptr = new_node.get();
  new_node->prev = node;
  new_node->next = std::move(node->next);
  if (new_node->next) {
    new_node->next->prev = new_node_ptr;
  } else {
    tail_ = new_node_ptr;
  }
  node->next = std::move(new_node);
  ++node_count_;
  return new_node_ptr;
}

void QuickList::MergeNext(Node* left) {
  if (!CanMergeNodes(left, left == nullptr ? nullptr : left->next.get())) {
    return;
  }

  Node* right = left->next.get();
  const size_t right_size = right->listpack->Size();
  if (right_size > 0 && !right->listpack->ForEach(
                            0, right_size - 1, [left](std::string_view value) {
                              return left->listpack->Append(value);
                            })) {
    return;
  }
  DeleteNode(right);
}

void QuickList::MergeAll() {
  for (Node* node = head_.get(); node != nullptr; node = node->next.get()) {
    while (CanMergeNodes(node, node->next.get())) {
      MergeNext(node);
    }
  }
}

QuickList::Node* QuickList::AppendNode() {
  auto node = std::make_unique<Node>();
  Node* node_ptr = node.get();
  if (!head_) {
    head_ = std::move(node);
    tail_ = node_ptr;
  } else {
    assert(tail_ != nullptr);
    if (tail_ == nullptr) {
      return nullptr;
    }
    node->prev = tail_;
    tail_->next = std::move(node);
    tail_ = node_ptr;
  }
  ++node_count_;
  return node_ptr;
}

QuickList::Node* QuickList::PrependNode() {
  auto node = std::make_unique<Node>();
  Node* node_ptr = node.get();
  if (!head_) {
    head_ = std::move(node);
    tail_ = node_ptr;
  } else {
    node->next = std::move(head_);
    node->next->prev = node_ptr;
    head_ = std::move(node);
  }
  ++node_count_;
  return node_ptr;
}

void QuickList::DeleteNode(Node* node) {
  if (node == nullptr) {
    return;
  }

  if (node == head_.get()) {
    head_ = std::move(head_->next);
    if (head_) {
      head_->prev = nullptr;
    } else {
      tail_ = nullptr;
    }
  } else {
    Node* prev = node->prev;
    assert(prev != nullptr);
    if (prev == nullptr) {
      return;
    }
    prev->next = std::move(node->next);
    if (prev->next) {
      prev->next->prev = prev;
    } else {
      tail_ = prev;
    }
  }
  --node_count_;
}

void QuickList::Clear() {
  head_.reset();
  tail_ = nullptr;
  size_ = 0;
  node_count_ = 0;
}
}  // namespace redis_simple::in_memory
