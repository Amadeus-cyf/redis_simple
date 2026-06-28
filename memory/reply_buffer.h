#pragma once

#include <vector>

#include "buf_node.h"

namespace redis_simple::in_memory {
class ReplyBuffer {
 public:
  ReplyBuffer();
  const char* UnsentBuffer() const { return buf_ + sent_; }
  size_t UnsentLength() const { return size_ - sent_; }
  size_t SentLength() const { return sent_; }
  size_t ReplyCount() const { return node_count_; }
  size_t ReplyBytes() const { return reply_bytes_; }
  size_t BufferSize() const { return size_; }
  BufNode* ReplyHead() const { return reply_head_; }
  BufNode* ReplyTail() const { return reply_tail_; }
  size_t Append(const char* s, size_t len);
  void Consume(size_t nwritten);
  std::vector<std::pair<char*, size_t>> Blocks();
  bool Empty() const { return size_ == 0 && node_count_ == 0; }
  ~ReplyBuffer();

 private:
  static constexpr size_t kDefaultBufferSize = 4096;
  size_t AppendToBuffer(const char* s, size_t len);
  size_t AppendToList(const char* c, size_t len);
  size_t ConsumeBuffer(size_t nwritten);
  void ConsumeList(size_t nwritten);
  void ClearBuffer();
  void PushNode(BufNode* node);
  void DeleteNode(BufNode* node, BufNode* prev);
  BufNode* NewNode(const char* buffer, size_t len);
  static size_t AppendToNode(BufNode* node, const char* buffer, size_t len);
  char* buf_;
  size_t size_;
  size_t buf_usable_size_;
  BufNode* reply_head_{};
  BufNode* reply_tail_{};
  size_t node_count_{};
  size_t reply_bytes_{};
  size_t sent_;
};
}  // namespace redis_simple::in_memory
