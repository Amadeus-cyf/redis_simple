#include "memory/dynamic_buffer.h"

#include <gtest/gtest.h>

#include <memory>

namespace redis_simple::in_memory {
class DynamicBufferTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { buffer = std::make_unique<DynamicBuffer>(); }
  static void TearDownTestSuite() { buffer.reset(); }

  static std::unique_ptr<DynamicBuffer> buffer;
};

std::unique_ptr<DynamicBuffer> DynamicBufferTest::buffer = nullptr;

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
  buffer->Append(buf, 4096);
  ASSERT_EQ(buffer->Capacity(), 4096);
  ASSERT_EQ(buffer->Size(), 4096);
  ASSERT_EQ(buffer->Consumed(), 0);
}

TEST_F(DynamicBufferTest, ProcessInline) {
  const std::string& s = buffer->ReadLine();
  ASSERT_EQ(s.length(), 1023);
  ASSERT_EQ(s, std::string(1023, 'a'));
  ASSERT_EQ(buffer->Capacity(), 4096);
  ASSERT_EQ(buffer->Size(), 4096);
  ASSERT_EQ(buffer->Consumed(), 1024);

  const std::string& s1 = buffer->ReadLine();
  ASSERT_EQ(s1.length(), 1023);
  ASSERT_EQ(s1, std::string(1023, 'b'));
  ASSERT_EQ(buffer->Capacity(), 4096);
  ASSERT_EQ(buffer->Size(), 4096);
  ASSERT_EQ(buffer->Consumed(), 2048);

  buffer->Compact();
  ASSERT_EQ(buffer->Capacity(), 4096);
  ASSERT_EQ(buffer->Size(), 2048);
  ASSERT_EQ(buffer->Consumed(), 0);

  const std::string& s2 = buffer->ReadLine();
  ASSERT_EQ(s2.length(), 1023);
  ASSERT_EQ(s2, std::string(1023, 'c'));
}

TEST_F(DynamicBufferTest, TrimProcessed) {
  buffer->Compact();
  ASSERT_EQ(buffer->Capacity(), 4096);
  ASSERT_EQ(buffer->Size(), 1024);
  ASSERT_EQ(buffer->Consumed(), 0);
}

TEST_F(DynamicBufferTest, Resize) {
  char buf[8192];
  for (int i = 0; i < 8192; ++i) {
    buf[i] = (i + 1) % 1024 == 0 ? '\n' : 'c';
  }
  ASSERT_EQ(buffer->Size(), 1024);
  buffer->Append(buf, 8192);
  ASSERT_EQ(buffer->Capacity(), 18432);
  ASSERT_EQ(buffer->Size(), 9216);
  ASSERT_EQ(buffer->Consumed(), 0);
}
}  // namespace redis_simple::in_memory
