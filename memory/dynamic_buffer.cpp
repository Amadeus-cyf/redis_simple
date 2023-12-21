#include "dynamic_buffer.h"

#include <unistd.h>

#include "utils/string_utils.h"

namespace redis_simple {
namespace in_memory {
DynamicBuffer::DynamicBuffer()
    : buf_(new char[4096]), nread_(0), processed_offset_(0), len_(4096){};

void DynamicBuffer::WriteToBuffer(const char* buffer, size_t n) {
  if (n == 0) {
    return;
  }
  if (len_ - nread_ < n) {
    Resize(n + nread_);
  }
  memcpy(buf_ + nread_, buffer, n);
  nread_ += n;
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
  const std::string& s =
      std::string(buf_ + processed_offset_, c - buf_ - processed_offset_);
  // need to include \r\n
  processed_offset_ += (s.length() + offset);
  return s;
}

void DynamicBuffer::Resize(size_t n) {
  if (n * 2 < resizeThreshold) {
    n *= 2;
  } else {
    n += 10000;
  }
  char* newbuf = new char[n * 2];
  memcpy(newbuf, buf_, len_);
  delete[] buf_;
  buf_ = newbuf;
  len_ = n;
}

void DynamicBuffer::Clear() {
  memset(buf_, 0, len_);
  nread_ = 0;
  processed_offset_ = 0;
}
}  // namespace in_memory
}  // namespace redis_simple
