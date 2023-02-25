#pragma once

#include <string>

namespace redis_simple {
namespace in_memory {
class QueryBuffer {
 public:
  QueryBuffer();
  size_t getQueryLen() { return query_len; }
  size_t getQueryRead() { return query_read; }
  size_t getQueryOffset() { return query_off; }
  void writeToBuffer(char buf[], size_t n);
  void trimProcessedBuffer();
  std::string processInlineBuffer();
  ~QueryBuffer() {
    delete[] query_buf;
    query_buf = nullptr;
  }

 private:
  void resize(size_t n);
  char* query_buf;
  /* total length of the query buffer */
  size_t query_len;
  /* offset of which we have already read into the query_buffer */
  size_t query_read;
  /* offset of which we have processed the command */
  size_t query_off;
};
}  // namespace in_memory
}  // namespace redis_simple
