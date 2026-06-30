#include "dynamic_buffer.h"

#include <cstring>
#include <memory>

#include "utils/string_utils.h"

namespace redis_simple::in_memory {
DynamicBuffer::DynamicBuffer()
    : buf_(std::make_unique<char[]>(4096)),
      size_(0),
      processed_(0),
      capacity_(4096) {}

void DynamicBuffer::Append(const char* buffer, size_t n) {
  if (n > 0) {
    if (capacity_ - size_ < n) {
      Resize(n + size_);
    }
    std::memcpy(buf_.get() + size_, buffer, n);
    size_ += n;
  }
}

void DynamicBuffer::Compact() {
  if (processed_ == 0) {
    return;
  }
  utils::ShiftCString(buf_.get(), capacity_, processed_);
  size_ -= processed_;
  processed_ = 0;
}

std::string DynamicBuffer::ReadLine() {
  char* c = strchr(buf_.get() + processed_, '\n');
  if (c == nullptr) {
    return "";
  }
  int offset = 1;
  if (*(c - 1) == '\r') {
    --c;
    ++offset;
  }
  const size_t line_length = c - buf_.get() - processed_;
  std::string line(buf_.get() + processed_, line_length);
  processed_ += line_length + offset;
  return line;
}

void DynamicBuffer::Resize(size_t n) {
  if (n * 2 < kResizeThreshold) {
    n *= 2;
  } else {
    n += 1000;
  }
  auto new_buf = std::make_unique<char[]>(n * 2);
  std::memcpy(new_buf.get(), buf_.get(), capacity_);
  buf_ = std::move(new_buf);
  capacity_ = n;
}

void DynamicBuffer::Clear() {
  std::memset(buf_.get(), 0, capacity_);
  size_ = 0;
  processed_ = 0;
}
}  // namespace redis_simple::in_memory
