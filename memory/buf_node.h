#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace redis_simple::in_memory {
class BufNode {
 public:
  static std::unique_ptr<BufNode> Create(size_t len) {
    return std::unique_ptr<BufNode>(
        new BufNode(std::max(len, static_cast<size_t>(1024))));
  }
  static std::unique_ptr<BufNode> Create(std::string data) {
    return std::unique_ptr<BufNode>(new BufNode(std::move(data)));
  }
  char* Data() { return buf_ != nullptr ? buf_.get() : owned_.data(); }
  bool Writable() const { return buf_ != nullptr; }
  ~BufNode() = default;
  std::unique_ptr<char[]> buf_;
  size_t used_;
  size_t capacity_;
  std::unique_ptr<BufNode> next_;
  std::string owned_;

 private:
  static constexpr size_t kProtoNodeSize = 1024;
  explicit BufNode() : used_(0), capacity_(kProtoNodeSize) {
    buf_ = std::make_unique<char[]>(kProtoNodeSize);
  }
  explicit BufNode(size_t len) : used_(0), capacity_(len) {
    buf_ = std::make_unique<char[]>(len);
  }
  explicit BufNode(std::string data)
      : used_(data.size()),
        capacity_(data.capacity()),
        owned_(std::move(data)) {}
};
}  // namespace redis_simple::in_memory
