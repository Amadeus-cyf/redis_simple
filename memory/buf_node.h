#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace redis_simple::in_memory {
class BufNode {
 public:
  static BufNode* Create(size_t len) {
    return new BufNode(std::max(len, static_cast<size_t>(1024)));
  }
  ~BufNode() {
    if (buf_) {
      delete[] buf_;
      buf_ = nullptr;
    }
    if (next_) {
      next_ = nullptr;
    }
  }
  char* buf_;
  size_t used_;
  size_t len_;
  BufNode* next_;

 private:
  static constexpr size_t kProtoNodeSize = 1024;
  explicit BufNode() : len_(kProtoNodeSize), used_(0), next_(nullptr) {
    buf_ = new char[kProtoNodeSize];
    std::memset(buf_, '\0', kProtoNodeSize);
  }
  explicit BufNode(size_t len) : len_(len), used_(0), next_(nullptr) {
    buf_ = new char[len];
    std::memset(buf_, '\0', len);
  }
};
}  // namespace redis_simple::in_memory
