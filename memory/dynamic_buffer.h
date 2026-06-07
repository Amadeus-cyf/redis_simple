#pragma once

#include <string>

namespace redis_simple {
namespace in_memory {
class DynamicBuffer {
 public:
  DynamicBuffer();
  size_t Len() const { return len_; }
  size_t NRead() const { return nread_; }
  size_t ProcessedOffset() const { return processed_offset_; }
  void IncrProcessedOffset(size_t processsed) {
    processed_offset_ += processsed;
  }
  void WriteToBuffer(const char* buffer, size_t n);
  void TrimProcessedBuffer();
  std::string ProcessInlineBuffer();
  std::string GetBufInString() const { return std::string(buf_, nread_); }
  bool Empty() const { return nread_ == 0; }
  void Clear();
  ~DynamicBuffer() {
    delete[] buf_;
    buf_ = nullptr;
  }

 private:
  static const constexpr size_t kResizeThreshold = 1024 * 32;
  void Resize(size_t n);
  char* buf_;
  size_t len_;
  size_t nread_;
  size_t processed_offset_;
};
}  // namespace in_memory
}  // namespace redis_simple
