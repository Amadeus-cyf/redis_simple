#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
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
  explicit BufNode() : capacity_(kProtoNodeSize), used_(0) {
    buf_ = std::make_unique<char[]>(kProtoNodeSize);
    std::memset(buf_.get(), '\0', kProtoNodeSize);
  }
  explicit BufNode(size_t len) : capacity_(len), used_(0) {
    buf_ = std::make_unique<char[]>(len);
    std::memset(buf_.get(), '\0', len);
  }
};
}  // namespace redis_simple::in_memory
