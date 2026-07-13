#include "memory/reply_buffer.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

namespace redis_simple::in_memory {
namespace {
constexpr size_t kExpectedSentAfterConsume = 5000 - (4096 - 2047) - 2000;
constexpr size_t kTailInitialUsed = 1000 / 3;
constexpr size_t kTailFillBytes = 1024 - kTailInitialUsed;
constexpr size_t kRemainingBytes = 5000 - kTailFillBytes;

std::string BlockString(const iovec& block) {
  return {static_cast<const char*>(block.iov_base), block.iov_len};
}

std::unique_ptr<ReplyBuffer> MakeBufferWithMainBytes() {
  auto buf = std::make_unique<ReplyBuffer>();
  std::string s(2000, 'a');
  buf->Append(s.c_str(), 2000);
  return buf;
}

std::unique_ptr<ReplyBuffer> MakeBufferWithReplyList() {
  auto buf = MakeBufferWithMainBytes();
  std::string s1(4096, 'b');
  buf->Append(s1.c_str(), 4096);
  std::string s2(1000, 'c');
  buf->Append(s2.c_str(), 1000);
  std::string s3(24, 'c');
  buf->Append(s3.c_str(), 24);
  std::string s4(1024, 'c');
  buf->Append(s4.c_str(), 1000);
  std::string s5(1024, 'd');
  buf->Append(s5.c_str(), 1024);
  return buf;
}

std::unique_ptr<ReplyBuffer> MakeConsumedBuffer() {
  auto buf = MakeBufferWithReplyList();
  buf->Consume(2047);
  buf->Consume(5000);
  return buf;
}
}  // namespace

TEST(ReplyBufferTest, AddToBuf) {
  auto buf = MakeBufferWithMainBytes();
  ASSERT_EQ(buf->BufferSize(), 2000);
}

TEST(ReplyBufferTest, AddToReplyList) {
  auto buf = MakeBufferWithMainBytes();
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
  ASSERT_EQ(BlockString(mem_vec[0]),
            std::string(2000, 'a').append(4096 - 2000, 'b'));
  ASSERT_EQ(mem_vec[0].iov_len, 4096);
  ASSERT_EQ(BlockString(mem_vec[1]), std::string(4096 - (4096 - 2000), 'b'));
  ASSERT_EQ(mem_vec[1].iov_len, 4096 - (4096 - 2000));
  ASSERT_EQ(BlockString(mem_vec[2]), std::string(1024, 'c'));
  ASSERT_EQ(mem_vec[2].iov_len, 1024);
  ASSERT_EQ(BlockString(mem_vec[3]),
            std::string(1000, 'c') + std::string(24, 'd'));
  ASSERT_EQ(mem_vec[3].iov_len, 1024);
  ASSERT_EQ(BlockString(mem_vec[4]), std::string(1000, 'd'));
  ASSERT_EQ(mem_vec[4].iov_len, 1000);
}

TEST(ReplyBufferTest, Consume) {
  auto buf = MakeBufferWithReplyList();
  // Partially consume the main buffer.
  buf->Consume(2047);
  ASSERT_EQ(buf->BufferSize(), 4096);
  ASSERT_EQ(buf->SentLength(), 2047);

  // Consume the rest of the main buffer and one list node.
  buf->Consume(5000);
  ASSERT_EQ(buf->BufferSize(), 0);
  ASSERT_EQ(buf->SentLength(), kExpectedSentAfterConsume);
  ASSERT_EQ(buf->ReplyCount(), 3);

  const auto mem_vec = buf->Blocks();
  ASSERT_EQ(mem_vec.size(), 3);
  ASSERT_EQ(BlockString(mem_vec[0]),
            std::string(1024 - kExpectedSentAfterConsume, 'c'));
  ASSERT_EQ(mem_vec[0].iov_len, 1024 - kExpectedSentAfterConsume);
  ASSERT_EQ(BlockString(mem_vec[1]),
            std::string(1000, 'c') + std::string(24, 'd'));
  ASSERT_EQ(mem_vec[1].iov_len, 1024);
  ASSERT_EQ(BlockString(mem_vec[2]), std::string(1000, 'd'));
  ASSERT_EQ(mem_vec[2].iov_len, 1000);
}

TEST(ReplyBufferTest, ConsumeListNodeInMultiplePartialWrites) {
  auto buf = MakeBufferWithReplyList();
  buf->Consume(4096);

  buf->Consume(300);
  ASSERT_EQ(buf->SentLength(), 300);
  auto mem_vec = buf->Blocks();
  ASSERT_EQ(mem_vec.size(), 4);
  ASSERT_EQ(mem_vec[0].iov_len, 1700);

  buf->Consume(400);
  ASSERT_EQ(buf->SentLength(), 700);
  mem_vec = buf->Blocks();
  ASSERT_EQ(mem_vec.size(), 4);
  ASSERT_EQ(BlockString(mem_vec[0]), std::string(1300, 'b'));
  ASSERT_EQ(mem_vec[0].iov_len, 1300);
}

TEST(ReplyBufferTest, AppendNewNodeToReplyList) {
  auto buf = MakeConsumedBuffer();
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
  ASSERT_EQ(BlockString(mem_vec[0]),
            std::string(1024 - kExpectedSentAfterConsume, 'c'));
  ASSERT_EQ(mem_vec[0].iov_len, 1024 - kExpectedSentAfterConsume);
  ASSERT_EQ(BlockString(mem_vec[1]),
            std::string(1000, 'c') + std::string(24, 'd'));
  ASSERT_EQ(mem_vec[1].iov_len, 1024);
  ASSERT_EQ(BlockString(mem_vec[2]),
            std::string(kTailInitialUsed, 'd').append(kTailFillBytes, 'e'));
  ASSERT_EQ(mem_vec[2].iov_len, 1024);
  ASSERT_EQ(BlockString(mem_vec[3]), std::string(kRemainingBytes, 'e'));
  ASSERT_EQ(mem_vec[3].iov_len, kRemainingBytes);
}
}  // namespace redis_simple::in_memory
