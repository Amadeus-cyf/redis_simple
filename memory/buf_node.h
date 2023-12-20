#pragma once

#include <memory>

namespace redis_simple {
namespace in_memory {
class BufNode {
 public:
  static BufNode* Create(const size_t len) {
    return new BufNode(std::max(len, static_cast<size_t>(1024)));
  }
  ~BufNode() {
    if (buf) {
      delete[] buf;
      buf = nullptr;
    }
    if (next) {
      next = nullptr;
    }
  }
  /* buffer */
  char* buf;
  /* size already written with reply */
  size_t used;
  /* total buffer len */
  size_t len;
  /* pointer to next buffer */
  BufNode* next;

 private:
  static constexpr const size_t ProtoNodeSize = 1024;
  explicit BufNode() : len(ProtoNodeSize), used(0), next(nullptr) {
    buf = new char[ProtoNodeSize];
    memset(buf, '\0', ProtoNodeSize);
  };
  explicit BufNode(size_t len) : len(len), used(0), next(nullptr) {
    buf = new char[len];
    memset(buf, '\0', len);
  }
};
}  // namespace in_memory
}  // namespace redis_simple
