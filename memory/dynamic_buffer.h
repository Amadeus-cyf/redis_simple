#pragma once

#include <string>

namespace redis_simple {
namespace in_memory {
class DynamicBuffer {
 public:
  DynamicBuffer();
  size_t Len() { return len; }
  size_t NRead() { return nread; }
  size_t ProcessedOffset() { return processed_offset; }
  void IncrProcessedOffset(size_t processsed) {
    processed_offset += processsed;
  }
  void WriteToBuffer(const char* buffer, size_t n);
  void TrimProcessedBuffer();
  std::string ProcessInlineBuffer();
  std::string GetBufInString() { return std::string(buf, nread); }
  bool Empty() { return nread == 0; }
  void Clear();
  ~DynamicBuffer() {
    delete[] buf;
    buf = nullptr;
  }

 private:
  static const constexpr size_t resizeThreshold = 1024 * 32;
  void Resize(size_t n);
  char* buf;
  /* total length of the query buffer */
  size_t len;
  /* offset of which we have already read into the query_buffer */
  size_t nread;
  /* offset of which we have processed the command */
  size_t processed_offset;
};
}  // namespace in_memory
}  // namespace redis_simple
