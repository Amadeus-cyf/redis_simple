#include "dynamic_buffer.h"

#include <cstring>

#include "utils/string_utils.h"

namespace redis_simple::in_memory {
DynamicBuffer::DynamicBuffer()
    : buf_(new char[4096]), size_(0), processed_(0), capacity_(4096) {}

void DynamicBuffer::Append(const char* buffer, size_t n) {
  if (n > 0) {
    if (capacity_ - size_ < n) {
      Resize(n + size_);
    }
    std::memcpy(buf_ + size_, buffer, n);
    size_ += n;
  }
}

void DynamicBuffer::Compact() {
  if (processed_ == 0) {
    return;
  }
  utils::ShiftCString(buf_, capacity_, processed_);
  size_ -= processed_;
  processed_ = 0;
}

std::string DynamicBuffer::ReadLine() {
  char* c = strchr(buf_ + processed_, '\n');
  if (c == nullptr) {
    return "";
  }
  int offset = 1;
  if (*(c - 1) == '\r') {
    --c;
    ++offset;
  }
  const size_t line_length = c - buf_ - processed_;
  std::string line(buf_ + processed_, line_length);
  processed_ += line_length + offset;
  return line;
}

void DynamicBuffer::Resize(size_t n) {
  if (n * 2 < kResizeThreshold) {
    n *= 2;
  } else {
    n += 1000;
  }
  char* newbuf = new char[n * 2];
  std::memcpy(newbuf, buf_, capacity_);
  delete[] buf_;
  buf_ = newbuf;
  capacity_ = n;
}

void DynamicBuffer::Clear() {
  std::memset(buf_, 0, capacity_);
  size_ = 0;
  processed_ = 0;
}
}  // namespace redis_simple::in_memory
