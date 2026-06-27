#pragma once

#include <vector>

#include "buf_node.h"

namespace redis_simple::in_memory {
class ReplyBuffer {
 public:
  ReplyBuffer();
  const char* UnsentBuffer() const { return buf_ + sent_len_; }
  size_t UnsentBufferLength() const { return buf_pos_ - sent_len_; }
  size_t SentLen() const { return sent_len_; }
  size_t ReplyLen() const { return reply_len_; }
  size_t ReplyBytes() const { return reply_bytes_; }
  size_t BufPosition() const { return buf_pos_; }
  BufNode* ReplyHead() const { return reply_head_; }
  BufNode* ReplyTail() const { return reply_tail_; }
  size_t AddReplyToBufferOrList(const char* s, size_t len);
  void ClearProcessed(size_t nwritten);
  std::vector<std::pair<char*, size_t>> Memvec();
  bool Empty() const { return buf_pos_ == 0 && reply_len_ == 0; }
  ~ReplyBuffer();

 private:
  static constexpr size_t kDefaultBufferSize = 4096;
  size_t AddReplyToBuffer(const char* s, size_t len);
  size_t AddReplyProtoToList(const char* c, size_t len);
  size_t ClearBufferProcessed(size_t nwritten);
  void ClearListProcessed(size_t nwritten);
  void ClearBuffer();
  void AddNodeToReplyList(BufNode* node);
  void DeleteNodeFromReplyList(BufNode* node, BufNode* prev);
  BufNode* CreateReplyNode(const char* buffer, size_t len);
  static size_t AppendToReplyNode(BufNode* node, const char* buffer,
                                  size_t len);
  char* buf_;
  size_t buf_pos_;
  size_t buf_usable_size_;
  BufNode* reply_head_{};
  BufNode* reply_tail_{};
  size_t reply_len_{};
  size_t reply_bytes_{};
  size_t sent_len_;
};
}  // namespace redis_simple::in_memory
