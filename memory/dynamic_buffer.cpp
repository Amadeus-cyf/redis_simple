#include "dynamic_buffer.h"

#include <unistd.h>

#include "utils/string_utils.h"

namespace redis_simple {
namespace in_memory {
DynamicBuffer::DynamicBuffer()
    : buf(new char[4096]), nread(0), processed_offset(0), len(4096){};

void DynamicBuffer::writeToBuffer(const char* buffer, size_t n) {
  if (len - nread < n) {
    resize(n + nread);
  }
  memcpy(buf + nread, buffer, n);
  nread += n;
}

void DynamicBuffer::trimProcessedBuffer() {
  if (processed_offset == 0) return;
  utils::shiftCStr(buf, len, processed_offset);
  nread -= processed_offset;
  processed_offset = 0;
}

std::string DynamicBuffer::processInlineBuffer() {
  printf("process input inline %zu %zu\n", processed_offset, nread);
  char* c = strchr(buf + processed_offset, '\n');
  if (!c) {
    printf("no newline found\n");
    return "";
  }
  int offset = 1;
  if (*(c - 1) == '\r') {
    --c;
    ++offset;
  }
  const std::string& s =
      std::string(buf + processed_offset, c - buf - processed_offset);
  // need to include \r\n
  processed_offset += (s.length() + offset);
  return s;
}

void DynamicBuffer::resize(size_t n) {
  if (n * 2 < resizeThreshold) {
    n *= 2;
  } else {
    n += 10000;
  }
  char* newbuf = new char[n * 2];
  memcpy(newbuf, buf, len);
  delete[] buf;
  buf = newbuf;
  len = n;
}

void DynamicBuffer::clear() {
  memset(buf, 0, len);
  nread = 0;
  processed_offset = 0;
}
}  // namespace in_memory
}  // namespace redis_simple
