#include "memory/reply_buffer.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

namespace redis_simple::in_memory {
class ReplyBufferTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { buf = std::make_unique<ReplyBuffer>(); }
  static void TearDownTestSuite() { buf.reset(); }

  static std::unique_ptr<ReplyBuffer> buf;
};

std::unique_ptr<ReplyBuffer> ReplyBufferTest::buf = nullptr;

TEST_F(ReplyBufferTest, AddToBuf) {
  std::string s(2000, 'a');
  size_t r = buf->Append(s.c_str(), 2000);
  ASSERT_EQ(r, 2000);
  ASSERT_EQ(buf->BufferSize(), 2000);
}

TEST_F(ReplyBufferTest, AddToReplyList) {
  std::string s1(4096, 'b');
  size_t r = buf->Append(s1.c_str(), 4096);
  ASSERT_EQ(r, 4096);
  ASSERT_EQ(buf->SentLength(), 0);
  ASSERT_EQ(buf->BufferSize(), 4096);
  ASSERT_EQ(buf->ReplyCount(), 1);

  // Add a new node to the reply list.
  std::string s2(1000, 'c');
  r = buf->Append(s2.c_str(), 1000);
  ASSERT_EQ(r, 1000);
  ASSERT_EQ(buf->ReplyCount(), 2);

  // no new node created
  std::string s3(24, 'c');
  r = buf->Append(s3.c_str(), 24);
  ASSERT_EQ(r, 24);
  ASSERT_EQ(buf->ReplyCount(), 2);

  // Add a new node with available space remained.
  std::string s4(1024, 'c');
  r = buf->Append(s4.c_str(), 1000);
  ASSERT_EQ(r, 1000);
  ASSERT_EQ(buf->BufferSize(), 4096);
  ASSERT_EQ(buf->ReplyCount(), 3);

  // Partially append to the last node and add a new node for the remaining
  // memory.
  std::string s5(1024, 'd');
  r = buf->Append(s5.c_str(), 1024);
  ASSERT_EQ(r, 1024);
  ASSERT_EQ(buf->BufferSize(), 4096);
  ASSERT_EQ(buf->ReplyCount(), 4);

  const auto mem_vec = buf->Blocks();
  ASSERT_EQ(mem_vec.size(), 5);
  ASSERT_EQ(std::string(mem_vec[0].first, mem_vec[0].second),
            std::string(2000, 'a').append(4096 - 2000, 'b'));
  ASSERT_EQ(mem_vec[0].second, 4096);
  ASSERT_EQ(std::string(mem_vec[1].first, mem_vec[1].second),
            std::string(4096 - (4096 - 2000), 'b'));
  ASSERT_EQ(mem_vec[1].second, 4096 - (4096 - 2000));
  ASSERT_EQ(std::string(mem_vec[2].first, mem_vec[2].second),
            std::string(1024, 'c'));
  ASSERT_EQ(mem_vec[2].second, 1024);
  ASSERT_EQ(std::string(mem_vec[3].first, mem_vec[3].second),
            std::string(1000, 'c') + std::string(24, 'd'));
  ASSERT_EQ(mem_vec[3].second, 1024);
  ASSERT_EQ(std::string(mem_vec[4].first, mem_vec[4].second),
            std::string(1000, 'd'));
  ASSERT_EQ(mem_vec[4].second, 1000);
}

TEST_F(ReplyBufferTest, Consume) {
  // Partially consume the main buffer.
  buf->Consume(2047);
  ASSERT_EQ(buf->BufferSize(), 4096);
  ASSERT_EQ(buf->SentLength(), 2047);

  // Consume the rest of the main buffer and one list node.
  buf->Consume(5000);
  ASSERT_EQ(buf->BufferSize(), 0);
  ASSERT_EQ(buf->SentLength(), 5000 - (4096 - 2047) - 2000);
  ASSERT_EQ(buf->ReplyCount(), 3);

  const auto mem_vec = buf->Blocks();
  ASSERT_EQ(mem_vec.size(), 3);
  ASSERT_EQ(std::string(mem_vec[0].first, mem_vec[0].second),
            std::string(1024 - (5000 - (4096 - 2047) - 2000), 'c'));
  ASSERT_EQ(mem_vec[0].second, 1024 - (5000 - (4096 - 2047) - 2000));
  ASSERT_EQ(std::string(mem_vec[1].first, mem_vec[1].second),
            std::string(1000, 'c') + std::string(24, 'd'));
  ASSERT_EQ(mem_vec[1].second, 1024);
  ASSERT_EQ(std::string(mem_vec[2].first, mem_vec[2].second),
            std::string(1000, 'd'));
  ASSERT_EQ(mem_vec[2].second, 1000);
}

TEST_F(ReplyBufferTest, AppendNewNodeToReplyList) {
  BufNode* tail = buf->ReplyTail();

  tail->used_ /= 3;
  std::memset(tail->buf_.get() + tail->used_, 0, tail->capacity_ - tail->used_);

  std::string s(5000, 'e');
  size_t r = buf->Append(s.c_str(), 5000);
  ASSERT_EQ(r, 5000);
  ASSERT_EQ(buf->BufferSize(), 0);
  ASSERT_EQ(buf->ReplyCount(), 4);

  const auto mem_vec = buf->Blocks();
  ASSERT_EQ(mem_vec.size(), 4);
  ASSERT_EQ(std::string(mem_vec[0].first, mem_vec[0].second),
            std::string(1024 - (5000 - (4096 - 2047) - 2000), 'c'));
  ASSERT_EQ(mem_vec[0].second, 1024 - (5000 - (4096 - 2047) - 2000));
  ASSERT_EQ(std::string(mem_vec[1].first, mem_vec[1].second),
            std::string(1000, 'c') + std::string(24, 'd'));
  ASSERT_EQ(mem_vec[1].second, 1024);
  ASSERT_EQ(std::string(mem_vec[2].first, mem_vec[2].second),
            std::string(1000 / 3, 'd').append(1024 - 1000 / 3, 'e'));
  ASSERT_EQ(mem_vec[2].second, 1024);
  ASSERT_EQ(std::string(mem_vec[3].first, mem_vec[3].second),
            std::string(5000 - (1024 - 1000 / 3), 'e'));
  ASSERT_EQ(mem_vec[3].second, 5000 - (1024 - 1000 / 3));
}
}  // namespace redis_simple::in_memory
