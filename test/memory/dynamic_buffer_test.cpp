#include "memory/dynamic_buffer.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace in_memory {
DynamicBuffer* buffer = new DynamicBuffer();

TEST(DynamicBufferTest, Write) {
  char buf[4096];
  char c = 'a';
  for (int i = 0; i < 4096; ++i) {
    if ((i + 1) % 1024 == 0) {
      ++c;
      buf[i] = '\n';
    } else {
      buf[i] = c;
    }
  }
  buffer->writeToBuffer(buf, 4096);
  ASSERT_EQ(buffer->getLen(), 4096);
  ASSERT_EQ(buffer->getRead(), 4096);
  ASSERT_EQ(buffer->getProcessedOffset(), 0);
}

TEST(DynamicBufferTest, ProcessInline) {
  const std::string& s = buffer->processInlineBuffer();
  ASSERT_EQ(s.length(), 1023);
  ASSERT_EQ(s, std::string(1023, 'a'));
  ASSERT_EQ(buffer->getLen(), 4096);
  ASSERT_EQ(buffer->getRead(), 4096);
  ASSERT_EQ(buffer->getProcessedOffset(), 1024);

  const std::string& s1 = buffer->processInlineBuffer();
  ASSERT_EQ(s1.length(), 1023);
  ASSERT_EQ(s1, std::string(1023, 'b'));
  ASSERT_EQ(buffer->getLen(), 4096);
  ASSERT_EQ(buffer->getRead(), 4096);
  ASSERT_EQ(buffer->getProcessedOffset(), 2048);

  buffer->trimProcessedBuffer();
  ASSERT_EQ(buffer->getLen(), 4096);
  ASSERT_EQ(buffer->getRead(), 2048);
  ASSERT_EQ(buffer->getProcessedOffset(), 0);

  const std::string& s2 = buffer->processInlineBuffer();
  ASSERT_EQ(s2.length(), 1023);
  ASSERT_EQ(s2, std::string(1023, 'c'));
}

TEST(DynamicBufferTest, TrimProcessed) {
  buffer->trimProcessedBuffer();
  ASSERT_EQ(buffer->getLen(), 4096);
  ASSERT_EQ(buffer->getRead(), 1024);
  ASSERT_EQ(buffer->getProcessedOffset(), 0);
}

TEST(DynamicBufferTest, Resize) {
  char buf[8192];
  for (int i = 0; i < 8192; ++i) {
    buf[i] = (i + 1) % 1024 == 0 ? '\n' : 'c';
  }
  ASSERT_EQ(buffer->getRead(), 1024);
  buffer->writeToBuffer(buf, 8192);
  ASSERT_EQ(buffer->getLen(), 18432);
  ASSERT_EQ(buffer->getRead(), 9216);
  ASSERT_EQ(buffer->getProcessedOffset(), 0);
}
}  // namespace in_memory
}  // namespace redis_simple
