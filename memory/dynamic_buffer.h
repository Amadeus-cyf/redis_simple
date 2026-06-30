#pragma once

#include <memory>
#include <string>

namespace redis_simple::in_memory {
class DynamicBuffer {
 public:
  DynamicBuffer();
  size_t Capacity() const { return capacity_; }
  size_t Size() const { return size_; }
  size_t Consumed() const { return processed_; }
  void Consume(size_t processed) { processed_ += processed; }
  void Append(const char* buffer, size_t n);
  void Compact();
  std::string ReadLine();
  std::string ToString() const { return std::string(buf_.get(), size_); }
  bool Empty() const { return size_ == 0; }
  void Clear();
  ~DynamicBuffer() = default;

 private:
  static constexpr size_t kResizeThreshold = 1024 * 32;
  void Resize(size_t n);
  std::unique_ptr<char[]> buf_;
  size_t capacity_;
  size_t size_;
  size_t processed_;
};
}  // namespace redis_simple::in_memory
