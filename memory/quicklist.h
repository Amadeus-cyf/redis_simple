#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "memory/listpack.h"

namespace redis_simple {
namespace in_memory {
class QuickList {
 public:
  static constexpr size_t kDefaultNodeMaxBytes = 8192;

  QuickList();
  explicit QuickList(size_t node_max_bytes);
  QuickList(const QuickList&) = delete;
  QuickList& operator=(const QuickList&) = delete;

  bool LPush(const std::string& value);
  bool RPush(const std::string& value);
  std::optional<std::string> LPop();
  std::optional<std::string> RPop();

  bool Empty() const { return size_ == 0; }
  size_t Size() const { return size_; }
  size_t NodeCount() const { return node_count_; }

 private:
  struct Node {
    Node();

    std::unique_ptr<ListPack> listpack;
    std::unique_ptr<Node> next;
    Node* prev;
  };

  bool PushToHeadNode(const std::string& value);
  bool PushToTailNode(const std::string& value);
  bool CanAppendToNode(const Node* node, const std::string& value) const;
  Node* AppendNode();
  Node* PrependNode();
  void DeleteNode(Node* node);

  std::unique_ptr<Node> head_;
  Node* tail_;
  size_t size_;
  size_t node_count_;
  size_t node_max_bytes_;
};
}  // namespace in_memory
}  // namespace redis_simple
