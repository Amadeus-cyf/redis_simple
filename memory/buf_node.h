#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>

namespace redis_simple::in_memory {
class BufNode {
 public:
  static std::unique_ptr<BufNode> Create(size_t len) {
    return std::unique_ptr<BufNode>(
        new BufNode(std::max(len, static_cast<size_t>(1024))));
  }
  ~BufNode() = default;
  std::unique_ptr<char[]> buf_;
  size_t used_;
  size_t capacity_;
  std::unique_ptr<BufNode> next_;

 private:
  static constexpr size_t kProtoNodeSize = 1024;
  explicit BufNode() : used_(0), capacity_(kProtoNodeSize) {
    buf_ = std::make_unique<char[]>(kProtoNodeSize);
  }
  explicit BufNode(size_t len) : used_(0), capacity_(len) {
    buf_ = std::make_unique<char[]>(len);
  }
};
}  // namespace redis_simple::in_memory
