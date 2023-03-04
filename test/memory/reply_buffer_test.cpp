#include "server/memory/reply_buffer.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace redis_simple {
namespace in_memory {
ReplyBuffer* buf = new ReplyBuffer();

TEST(ReplyBufferTest, AddToBuf) {
  std::string s(2000, 'a');
  size_t r = buf->addReplyToBufferOrList(s.c_str(), 2000);
  ASSERT_EQ(r, 2000);
  ASSERT_EQ(buf->getBufPos(), 2000);
}

TEST(ReplyBufferTest, AddToReplyList) {
  std::string s1(4096, 'b');
  size_t r = buf->addReplyToBufferOrList(s1.c_str(), 4096);
  ASSERT_EQ(r, 4096);

  std::string s2(2048, 'c');
  r = buf->addReplyToBufferOrList(s2.c_str(), 2048);
  ASSERT_EQ(r, 2048);

  std::string s3(1024, 'd');
  r = buf->addReplyToBufferOrList(s3.c_str(), 1024);
  ASSERT_EQ(r, 1024);

  ASSERT_EQ(buf->getBufPos(), 4096);
  ASSERT_EQ(buf->getReplyLen(), 3);

  const std::vector<std::pair<char*, size_t>> mem_vec = buf->getMemvec();

  for (const auto& p : mem_vec) {
    printf("%c %zu\n", p.first[0], p.second);
  }
}

TEST(ReplyBufferTest, TrimProcessedBuffer) {
  buf->writeProcessed(8000);
  ASSERT_EQ(buf->getBufPos(), 0);
  ASSERT_EQ(buf->getSentLen(), 8000 - 2000 - 4096);
  ASSERT_EQ(buf->getReplyLen(), 2);

  std::vector<std::pair<char*, size_t>> mem_vec = buf->getMemvec();
  for (const auto& p : mem_vec) {
    printf("%c %zu\n", p.first[0], p.second);
  }
}

TEST(ReplyBufferTest, AppendNewNodeToReplyList) {
  BufNode* tail = buf->getReplyTail();

  tail->used /= 3;
  memset(tail->buf + tail->used, 0, tail->len - tail->used);

  std::string s(5000, 'e');
  size_t r = buf->addReplyToBufferOrList(s.c_str(), 5000);
  ASSERT_EQ(r, 5000);
  ASSERT_EQ(buf->getBufPos(), 0);
  ASSERT_EQ(buf->getReplyLen(), 3);

  std::vector<std::pair<char*, size_t>> mem_vec_2 = buf->getMemvec();
  for (const auto& p : mem_vec_2) {
    printf("%c %c %zu\n", p.first[0], p.first[p.second - 1], p.second);
  }
}
}  // namespace in_memory
}  // namespace redis_simple
