#include "dynamic_buffer.h"

#include <cstring>
#include <memory>
#include <optional>
#include <string_view>

#include "utils/string_utils.h"

namespace redis_simple::in_memory {
DynamicBuffer::DynamicBuffer()
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays): contiguous byte buffer
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

std::optional<std::string_view> DynamicBuffer::ReadLineView() {
  const size_t readable = size_ - processed_;
  const auto* newline = static_cast<const char*>(
      std::memchr(buf_.get() + processed_, '\n', readable));
  if (newline == nullptr) {
    return std::nullopt;
  }
  const char* line_end = newline;
  int offset = 1;
  if (line_end > buf_.get() + processed_ && *(line_end - 1) == '\r') {
    --line_end;
    ++offset;
  }
  const size_t line_length = line_end - buf_.get() - processed_;
  std::string_view line(buf_.get() + processed_, line_length);
  processed_ += line_length + offset;
  return line;
}

std::string DynamicBuffer::ReadLine() {
  const auto line = ReadLineView();
  return line.has_value() ? std::string(*line) : "";
}

void DynamicBuffer::Resize(size_t n) {
  if (n * 2 < kResizeThreshold) {
    n *= 2;
  } else {
    n += 1000;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays): contiguous byte buffer
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
