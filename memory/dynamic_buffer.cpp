#include "dynamic_buffer.h"

#include <unistd.h>

#include "utils/string_utils.h"

namespace redis_simple {
namespace in_memory {
DynamicBuffer::DynamicBuffer()
    : buf_(new char[4096]), nread_(0), processed_offset_(0), len_(4096){};

void DynamicBuffer::WriteToBuffer(const char* buffer, size_t n) {
  if (n > 0) {
    if (len_ - nread_ < n) {
      Resize(n + nread_);
    }
    std::memcpy(buf_ + nread_, buffer, n);
    nread_ += n;
  }
}

void DynamicBuffer::TrimProcessedBuffer() {
  if (processed_offset_ == 0) return;
  utils::ShiftCStr(buf_, len_, processed_offset_);
  nread_ -= processed_offset_;
  processed_offset_ = 0;
}

std::string DynamicBuffer::ProcessInlineBuffer() {
  char* c = strchr(buf_ + processed_offset_, '\n');
  if (!c) {
    return "";
  }
  int offset = 1;
  if (*(c - 1) == '\r') {
    --c;
    ++offset;
  }
  const size_t line_length = c - buf_ - processed_offset_;
  std::string line(buf_ + processed_offset_, line_length);
  processed_offset_ += line_length + offset;
  return line;
}

void DynamicBuffer::Resize(size_t n) {
  if (n * 2 < kResizeThreshold) {
    n *= 2;
  } else {
    n += 1000;
  }
  char* newbuf = new char[n * 2];
  std::memcpy(newbuf, buf_, len_);
  delete[] buf_;
  buf_ = newbuf;
  len_ = n;
}

void DynamicBuffer::Clear() {
  std::memset(buf_, 0, len_);
  nread_ = 0;
  processed_offset_ = 0;
}
}  // namespace in_memory
}  // namespace redis_simple
