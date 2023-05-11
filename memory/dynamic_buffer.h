#pragma once

#include <string>

namespace redis_simple {
namespace in_memory {
class DynamicBuffer {
 public:
  DynamicBuffer();
  size_t getLen() { return len; }
  size_t getRead() { return nread; }
  size_t getProcessedOffset() { return processed_offset; }
  void incrProcessedOffset(size_t processsed) {
    processed_offset += processsed;
  }
  void writeToBuffer(const char* buffer, size_t n);
  void trimProcessedBuffer();
  std::string processInlineBuffer();
  std::string getBufInString() { return std::string(buf, nread); }
  bool isEmpty() { return nread == 0; }
  void clear();
  ~DynamicBuffer() {
    delete[] buf;
    buf = nullptr;
  }

 private:
  static const constexpr size_t resizeThreshold = 1024 * 32;
  void resize(size_t n);
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
