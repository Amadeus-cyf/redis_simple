#include "query_buffer.h"

#include <unistd.h>

#include "src/utils/string_utils.h"

namespace redis_simple {
namespace in_memory {
QueryBuffer::QueryBuffer()
    : query_buf(new char[4096]), query_read(0), query_off(0), query_len(4096){};

void QueryBuffer::writeToBuffer(char buf[], size_t n) {
  if (query_len - query_read < n) {
    resize((n + query_read) * 2);
  }

  memcpy(query_buf + query_read, buf, n);
  query_read += n;
}

void QueryBuffer::trimProcessedBuffer() {
  if (query_off == 0) return;
  utils::shiftCStr(query_buf, query_len, query_off);
  query_read -= query_off;
  query_off = 0;
}

std::string QueryBuffer::processInlineBuffer() {
  printf("process input inline %zu %zu\n", query_off, query_read);
  char* c = strchr(query_buf + query_off, '\n');
  if (!c) {
    printf("no newline found\n");
    return "";
  }
  const std::string& s =
      std::string(query_buf + query_off, c - query_buf - query_off);
  // need to include the newline
  query_off += (s.length() + 1);
  return s;
}

void QueryBuffer::resize(size_t n) {
  char* newbuf = new char[n * 2];
  memcpy(newbuf, query_buf, query_len);
  delete[] query_buf;
  query_buf = newbuf;
  query_len = n;
}
}  // namespace in_memory
}  // namespace redis_simple
