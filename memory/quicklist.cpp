#include "memory/quicklist.h"

#include <algorithm>

namespace redis_simple {
namespace in_memory {
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
                   static_cast<size_t>(ListPack::ListPackHeaderSize + 1))) {}

bool QuickList::LPush(const std::string& value) {
  if (!head_ || !CanAppendToNode(head_.get(), value)) {
    PrependNode();
  }
  if (!PushToHeadNode(value)) return false;
  ++size_;
  return true;
}

bool QuickList::RPush(const std::string& value) {
  if (!tail_ || !CanAppendToNode(tail_, value)) {
    AppendNode();
  }
  if (!PushToTailNode(value)) return false;
  ++size_;
  return true;
}

std::optional<std::string> QuickList::LPop() {
  if (!head_) return std::nullopt;

  const ssize_t idx = head_->listpack->First();
  auto value = head_->listpack->Get(idx);
  head_->listpack->Delete(idx);
  --size_;
  if (head_->listpack->Size() == 0) {
    DeleteNode(head_.get());
  }
  return value;
}

std::optional<std::string> QuickList::RPop() {
  if (!tail_) return std::nullopt;

  Node* node = tail_;
  const ssize_t idx = node->listpack->Last();
  auto value = node->listpack->Get(idx);
  node->listpack->Delete(idx);
  --size_;
  if (node->listpack->Size() == 0) {
    DeleteNode(node);
  }
  return value;
}

bool QuickList::PushToHeadNode(const std::string& value) {
  return head_ && head_->listpack->Prepend(value);
}

bool QuickList::PushToTailNode(const std::string& value) {
  return tail_ && tail_->listpack->Append(value);
}

bool QuickList::CanAppendToNode(const Node* node,
                                const std::string& value) const {
  if (!node) return false;
  const size_t estimated_bytes = value.size() + kEntryOverheadEstimate;
  return node->listpack->Size() == 0 ||
         node->listpack->GetTotalBytes() + estimated_bytes <= node_max_bytes_;
}

QuickList::Node* QuickList::AppendNode() {
  auto node = std::make_unique<Node>();
  Node* node_ptr = node.get();
  if (!head_) {
    head_ = std::move(node);
    tail_ = node_ptr;
  } else {
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
  if (!node) return;

  if (node == head_.get()) {
    head_ = std::move(head_->next);
    if (head_) {
      head_->prev = nullptr;
    } else {
      tail_ = nullptr;
    }
  } else {
    Node* prev = node->prev;
    prev->next = std::move(node->next);
    if (prev->next) {
      prev->next->prev = prev;
    } else {
      tail_ = prev;
    }
  }
  --node_count_;
}
}  // namespace in_memory
}  // namespace redis_simple
