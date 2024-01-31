#include "memory/reply_buffer.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace redis_simple {
namespace in_memory {
class ReplyBufferTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { buf = new ReplyBuffer(); }
  static void TearDownTestSuite() {
    delete buf;
    buf = nullptr;
  }
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
  ASSERT_EQ(buf->SentLen(), 0);
  ASSERT_EQ(buf->BufPosition(), 4096);
  ASSERT_EQ(buf->ReplyLen(), 1);

  // add a new node to the reply list
  std::string s2(1000, 'c');
  r = buf->AddReplyToBufferOrList(s2.c_str(), 1000);
  ASSERT_EQ(r, 1000);
  ASSERT_EQ(buf->ReplyLen(), 2);

  // no new node created
  std::string s3(24, 'c');
  r = buf->AddReplyToBufferOrList(s3.c_str(), 24);
  ASSERT_EQ(r, 24);
  ASSERT_EQ(buf->ReplyLen(), 2);

  // add a new node with available space remained
  std::string s4(1024, 'c');
  r = buf->AddReplyToBufferOrList(s4.c_str(), 1000);
  ASSERT_EQ(r, 1000);
  ASSERT_EQ(buf->BufPosition(), 4096);
  ASSERT_EQ(buf->ReplyLen(), 3);

  // partially append to the last node and add a new node for the remaining
  // memory
  std::string s5(1024, 'd');
  r = buf->AddReplyToBufferOrList(s5.c_str(), 1024);
  ASSERT_EQ(r, 1024);
  ASSERT_EQ(buf->BufPosition(), 4096);
  ASSERT_EQ(buf->ReplyLen(), 4);

  const std::vector<std::pair<char*, size_t>>& mem_vec = buf->Memvec();
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

TEST_F(ReplyBufferTest, ClearProcessed) {
  /* Partially clear the main buffer */
  buf->ClearProcessed(2047);
  ASSERT_EQ(buf->BufPosition(), 4096);
  ASSERT_EQ(buf->SentLen(), 2047);

  /* clear the entire buffer and one list node */
  buf->ClearProcessed(5000);
  ASSERT_EQ(buf->BufPosition(), 0);
  ASSERT_EQ(buf->SentLen(), 5000 - (4096 - 2047) - 2000);
  ASSERT_EQ(buf->ReplyLen(), 3);

  const std::vector<std::pair<char*, size_t>>& mem_vec = buf->Memvec();
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
  memset(tail->buf_ + tail->used_, 0, tail->len_ - tail->used_);

  std::string s(5000, 'e');
  size_t r = buf->AddReplyToBufferOrList(s.c_str(), 5000);
  ASSERT_EQ(r, 5000);
  ASSERT_EQ(buf->BufPosition(), 0);
  ASSERT_EQ(buf->ReplyLen(), 4);

  const std::vector<std::pair<char*, size_t>>& mem_vec = buf->Memvec();
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
}  // namespace in_memory
}  // namespace redis_simple
