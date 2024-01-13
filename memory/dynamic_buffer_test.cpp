#include "memory/dynamic_buffer.h"

#include <gtest/gtest.h>

namespace redis_simple {
namespace in_memory {
class DynamicBufferTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { buffer = new DynamicBuffer(); }
  static void TearDownTestSuite() {
    delete buffer;
    buffer = nullptr;
  }
  static DynamicBuffer* buffer;
};

DynamicBuffer* DynamicBufferTest::buffer = nullptr;

TEST_F(DynamicBufferTest, Write) {
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
  buffer->WriteToBuffer(buf, 4096);
  ASSERT_EQ(buffer->Len(), 4096);
  ASSERT_EQ(buffer->NRead(), 4096);
  ASSERT_EQ(buffer->ProcessedOffset(), 0);
}

TEST_F(DynamicBufferTest, ProcessInline) {
  const std::string& s = buffer->ProcessInlineBuffer();
  ASSERT_EQ(s.length(), 1023);
  ASSERT_EQ(s, std::string(1023, 'a'));
  ASSERT_EQ(buffer->Len(), 4096);
  ASSERT_EQ(buffer->NRead(), 4096);
  ASSERT_EQ(buffer->ProcessedOffset(), 1024);

  const std::string& s1 = buffer->ProcessInlineBuffer();
  ASSERT_EQ(s1.length(), 1023);
  ASSERT_EQ(s1, std::string(1023, 'b'));
  ASSERT_EQ(buffer->Len(), 4096);
  ASSERT_EQ(buffer->NRead(), 4096);
  ASSERT_EQ(buffer->ProcessedOffset(), 2048);

  buffer->TrimProcessedBuffer();
  ASSERT_EQ(buffer->Len(), 4096);
  ASSERT_EQ(buffer->NRead(), 2048);
  ASSERT_EQ(buffer->ProcessedOffset(), 0);

  const std::string& s2 = buffer->ProcessInlineBuffer();
  ASSERT_EQ(s2.length(), 1023);
  ASSERT_EQ(s2, std::string(1023, 'c'));
}

TEST_F(DynamicBufferTest, TrimProcessed) {
  buffer->TrimProcessedBuffer();
  ASSERT_EQ(buffer->Len(), 4096);
  ASSERT_EQ(buffer->NRead(), 1024);
  ASSERT_EQ(buffer->ProcessedOffset(), 0);
}

TEST_F(DynamicBufferTest, Resize) {
  char buf[8192];
  for (int i = 0; i < 8192; ++i) {
    buf[i] = (i + 1) % 1024 == 0 ? '\n' : 'c';
  }
  ASSERT_EQ(buffer->NRead(), 1024);
  buffer->WriteToBuffer(buf, 8192);
  ASSERT_EQ(buffer->Len(), 18432);
  ASSERT_EQ(buffer->NRead(), 9216);
  ASSERT_EQ(buffer->ProcessedOffset(), 0);
}
}  // namespace in_memory
}  // namespace redis_simple
