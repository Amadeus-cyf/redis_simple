#include "memory/quicklist.h"

#include <algorithm>
#include <cassert>

namespace redis_simple::in_memory {
namespace {
constexpr size_t kEntryOverheadEstimate = 8;
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

bool QuickList::LPush(const std::string& value) {
  if (!head_ || !CanAppendToNode(head_.get(), value)) {
    PrependNode();
  }
  if (!PushToHeadNode(value)) {
    return false;
  }
  ++size_;
  return true;
}

bool QuickList::RPush(const std::string& value) {
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

  size_t index = 0;
  for (const Node* node = head_.get(); node != nullptr;
       node = node->next.get()) {
    ssize_t listpack_index = node->listpack->First();
    while (listpack_index != -1 && index <= stop) {
      if (index >= start) {
        auto value = node->listpack->Get(static_cast<size_t>(listpack_index));
        if (value.has_value()) {
          values.push_back(*value);
        }
      }
      ++index;
      listpack_index =
          node->listpack->Next(static_cast<size_t>(listpack_index));
    }
    if (index > stop) {
      break;
    }
  }
  return values;
}

bool QuickList::PushToHeadNode(const std::string& value) {
  return head_ && head_->listpack->Prepend(value);
}

bool QuickList::PushToTailNode(const std::string& value) {
  return (tail_ != nullptr) && tail_->listpack->Append(value);
}

bool QuickList::CanAppendToNode(const Node* node,
                                const std::string& value) const {
  if (node == nullptr) {
    return false;
  }
  const size_t estimated_bytes = value.size() + kEntryOverheadEstimate;
  return node->listpack->Size() == 0 ||
         node->listpack->GetTotalBytes() + estimated_bytes <= node_max_bytes_;
}

bool QuickList::CanMergeNodes(const Node* left, const Node* right) const {
  if (left == nullptr || right == nullptr) {
    return false;
  }
  const size_t merged_bytes = left->listpack->GetTotalBytes() +
                              right->listpack->GetTotalBytes() -
                              ListPack::kListPackHeaderSize;
  return merged_bytes <= node_max_bytes_;
}

void QuickList::MergeNext(Node* left) {
  if (!CanMergeNodes(left, left == nullptr ? nullptr : left->next.get())) {
    return;
  }

  Node* right = left->next.get();
  ssize_t idx = right->listpack->First();
  while (idx != -1) {
    const auto value = right->listpack->Get(idx);
    if (value.has_value()) {
      left->listpack->Append(*value);
    }
    idx = right->listpack->Next(idx);
  }
  DeleteNode(right);
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
}  // namespace redis_simple::in_memory
