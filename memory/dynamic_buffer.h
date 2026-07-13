#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace redis_simple::in_memory {
class DynamicBuffer {
 public:
  DynamicBuffer();
  size_t Capacity() const { return capacity_; }
  size_t Size() const { return size_; }
  size_t Consumed() const { return processed_; }
  void Consume(size_t processed) {
    processed_ += std::min(processed, UnreadSize());
  }
  void Append(const char* buffer, size_t n);
  void Compact();
  std::optional<std::string_view> ReadLineView();
  std::string ReadLine();
  std::string_view View() const {
    return std::string_view(buf_.get() + processed_, UnreadSize());
  }
  std::string ToString() const { return std::string(View()); }
  bool Empty() const { return processed_ == size_; }
  void Clear();
  ~DynamicBuffer() = default;

 private:
  static constexpr size_t kResizeThreshold = 1024 * 32;
  size_t UnreadSize() const { return size_ - processed_; }
  void Resize(size_t n);
  std::unique_ptr<char[]> buf_;
  size_t capacity_;
  size_t size_;
  size_t processed_;
};
}  // namespace redis_simple::in_memory
