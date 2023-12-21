#include "memory/reply_buffer.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace redis_simple {
namespace in_memory {
class ReplyBufferTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { buf = new ReplyBuffer(); }
  static ReplyBuffer* buf;
};

ReplyBuffer* ReplyBufferTest::buf = nullptr;

TEST_F(ReplyBufferTest, AddToBuf) {
  std::string s(2000, 'a');
  size_t r = buf->AddReplyToBufferOrList(s.c_str(), 2000);
  ASSERT_EQ(r, 2000);
  ASSERT_EQ(buf->BufPosition(), 2000);
}

TEST_F(ReplyBufferTest, AddToReplyList) {
  std::string s1(4096, 'b');
  size_t r = buf->AddReplyToBufferOrList(s1.c_str(), 4096);
  ASSERT_EQ(r, 4096);

  std::string s2(2048, 'c');
  r = buf->AddReplyToBufferOrList(s2.c_str(), 2048);
  ASSERT_EQ(r, 2048);

  std::string s3(1024, 'd');
  r = buf->AddReplyToBufferOrList(s3.c_str(), 1024);
  ASSERT_EQ(r, 1024);

  ASSERT_EQ(buf->BufPosition(), 4096);
  ASSERT_EQ(buf->ReplyLen(), 3);

  const std::vector<std::pair<char*, size_t>>& mem_vec = buf->Memvec();

  for (const auto& p : mem_vec) {
    printf("%c %zu\n", p.first[0], p.second);
  }
}

TEST_F(ReplyBufferTest, TrimProcessedBuffer) {
  buf->ClearProcessed(8000);
  ASSERT_EQ(buf->BufPosition(), 0);
  ASSERT_EQ(buf->SentLen(), 8000 - 2000 - 4096);
  ASSERT_EQ(buf->ReplyLen(), 2);

  const std::vector<std::pair<char*, size_t>>& mem_vec = buf->Memvec();
  for (const auto& p : mem_vec) {
    printf("%c %zu\n", p.first[0], p.second);
  }
}

TEST_F(ReplyBufferTest, AppendNewNodeToReplyList) {
  BufNode* tail = buf->ReplyTail();

  tail->used_ /= 3;
  memset(tail->buf_ + tail->used_, 0, tail->len_ - tail->used_);

  std::string s(5000, 'e');
  size_t r = buf->AddReplyToBufferOrList(s.c_str(), 5000);
  ASSERT_EQ(r, 5000);
  ASSERT_EQ(buf->BufPosition(), 0);
  ASSERT_EQ(buf->ReplyLen(), 3);

  const std::vector<std::pair<char*, size_t>>& mem_vec_2 = buf->Memvec();
  for (const auto& p : mem_vec_2) {
    printf("%c %c %zu\n", p.first[0], p.first[p.second - 1], p.second);
  }
}
}  // namespace in_memory
}  // namespace redis_simple
