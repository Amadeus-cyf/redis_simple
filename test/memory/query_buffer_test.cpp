#include "src/memory/query_buffer.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace in_memory {
QueryBuffer* qb = new QueryBuffer();

TEST(QueryBufferTest, Write) {
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
  qb->writeToBuffer(buf, 4096);
  ASSERT_EQ(qb->getQueryLen(), 4096);
  ASSERT_EQ(qb->getQueryRead(), 4096);
  ASSERT_EQ(qb->getQueryOffset(), 0);
}

TEST(QueryBufferTest, ProcessInline) {
  const std::string& s = qb->processInlineBuffer();
  ASSERT_EQ(s.length(), 1023);
  ASSERT_EQ(s, std::string(1023, 'a'));
  ASSERT_EQ(qb->getQueryLen(), 4096);
  ASSERT_EQ(qb->getQueryRead(), 4096);
  ASSERT_EQ(qb->getQueryOffset(), 1024);

  const std::string& s1 = qb->processInlineBuffer();
  ASSERT_EQ(s1.length(), 1023);
  ASSERT_EQ(s1, std::string(1023, 'b'));
  ASSERT_EQ(qb->getQueryLen(), 4096);
  ASSERT_EQ(qb->getQueryRead(), 4096);
  ASSERT_EQ(qb->getQueryOffset(), 2048);

  qb->trimProcessedBuffer();
  ASSERT_EQ(qb->getQueryLen(), 4096);
  ASSERT_EQ(qb->getQueryRead(), 2048);
  ASSERT_EQ(qb->getQueryOffset(), 0);

  const std::string& s2 = qb->processInlineBuffer();
  ASSERT_EQ(s2.length(), 1023);
  ASSERT_EQ(s2, std::string(1023, 'c'));
}

TEST(QueryBufferTest, TrimProcessed) {
  qb->trimProcessedBuffer();
  ASSERT_EQ(qb->getQueryLen(), 4096);
  ASSERT_EQ(qb->getQueryRead(), 1024);
  ASSERT_EQ(qb->getQueryOffset(), 0);
}

TEST(QueryBufferTest, Resize) {
  char buf[8192];
  for (int i = 0; i < 8192; ++i) {
    buf[i] = (i + 1) % 1024 == 0 ? '\n' : 'c';
  }
  ASSERT_EQ(qb->getQueryRead(), 1024);
  qb->writeToBuffer(buf, 8192);
  ASSERT_EQ(qb->getQueryLen(), 18432);
  ASSERT_EQ(qb->getQueryRead(), 9216);
  ASSERT_EQ(qb->getQueryOffset(), 0);
}
}  // namespace in_memory
}  // namespace redis_simple
