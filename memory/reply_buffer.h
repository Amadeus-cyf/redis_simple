#pragma once

#include <vector>

#include "buf_node.h"

namespace redis_simple {
namespace in_memory {
class ReplyBuffer {
 public:
  ReplyBuffer();
  const char* UnsentBuffer() { return buf_ + sent_len_; }
  size_t UnsentBufferLength() { return buf_pos_ - sent_len_; }
  size_t SentLen() { return sent_len_; };
  size_t ReplyLen() { return reply_len_; };
  size_t ReplyBytes() { return reply_bytes_; }
  size_t BufPosition() { return buf_pos_; };
  BufNode* ReplyHead() { return reply_head_; }
  BufNode* ReplyTail() { return reply_tail_; }
  size_t AddReplyToBufferOrList(const char* s, size_t len);
  void ClearProcessed(size_t nwritten);
  std::vector<std::pair<char*, size_t>> Memvec();
  bool Empty() { return buf_pos_ == 0 && reply_len_ == 0; }
  ~ReplyBuffer();

 private:
  static constexpr const size_t defaultBufferSize = 4096;
  size_t AddReplyToBuffer(const char* s, size_t len);
  size_t AddReplyProtoToList(const char* c, size_t len);
  size_t ClearBufferProcessed(size_t nwritten);
  void ClearListProcessed(size_t nwritten);
  void ClearBuffer();
  void AddNodeToReplyList(BufNode* node);
  void DeleteNodeFromReplyList(BufNode* node, BufNode* prev);
  BufNode* CreateReplyNode(const char* buffer, size_t len);
  size_t AppendToReplyNode(BufNode* node, const char* buffer, size_t len);
  /* output main buffer */
  char* buf_;
  /* the end index of the main buffer which is already in use */
  size_t buf_pos_;
  /* remaining bytes in main buffer */
  size_t buf_usable_size_;
  /* head of the reply list */
  BufNode* reply_head_;
  /* tail of the reply list */
  BufNode* reply_tail_;
  /* number of buf nodes */
  size_t reply_len_;
  /* total size of buf nodes */
  size_t reply_bytes_;
  /* bytes sent to the client for current buffer (main buffer / node of reply
   * list) */
  size_t sent_len_;
};
}  // namespace in_memory
}  // namespace redis_simple
