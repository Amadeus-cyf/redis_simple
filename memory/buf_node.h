#pragma once

namespace redis_simple {
namespace in_memory {
class BufNode {
 public:
  static BufNode* Create(const size_t len) {
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
  /* buffer */
  char* buf_;
  /* size already written with reply */
  size_t used_;
  /* total buffer len */
  size_t len_;
  /* pointer to next buffer */
  BufNode* next_;

 private:
  static constexpr const size_t ProtoNodeSize = 1024;
  explicit BufNode() : len_(ProtoNodeSize), used_(0), next_(nullptr) {
    buf_ = new char[ProtoNodeSize];
    memset(buf_, '\0', ProtoNodeSize);
  };
  explicit BufNode(size_t len) : len_(len), used_(0), next_(nullptr) {
    buf_ = new char[len];
    memset(buf_, '\0', len);
  }
};
}  // namespace in_memory
}  // namespace redis_simple
