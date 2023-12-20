#pragma once

#include <vector>

#include "buf_node.h"

namespace redis_simple {
namespace in_memory {
class ReplyBuffer {
 public:
  ReplyBuffer();
  const char* UnsentBuffer() { return buf + sent_len; }
  size_t UnsentBufferLength() { return bufpos - sent_len; }
  size_t SentLen() { return sent_len; };
  size_t ReplyLen() { return reply_len; };
  size_t ReplyBytes() { return reply_bytes; }
  size_t BufPos() { return bufpos; };
  BufNode* ReplyHead() { return reply; }
  BufNode* ReplyTail() { return reply_tail; }
  size_t AddReplyToBufferOrList(const char* s, size_t len);
  void WriteProcessed(size_t nwritten);
  std::vector<std::pair<char*, size_t>> Memvec();
  void ClearBuf();
  bool Empty() { return bufpos == 0 && reply_len == 0; }
  ~ReplyBuffer();

 private:
  size_t AddReplyToBuffer(const char* s, size_t len);
  size_t AddReplyProtoToList(const char* c, size_t len);
  /* written only main buffer to the client */
  void _writeProcessed(size_t nwritten);
  /* written both main buffer and reply list to the client */
  void _writevProcessed(size_t nwritten);
  /* output main buffer */
  char* buf;
  /* buf position for bytes already in use in main buffer */
  size_t bufpos;
  /* remaining bytes in main buffer */
  size_t buf_usable_size;
  /* reply linked list */
  BufNode* reply;
  BufNode* reply_tail;
  /* number of buf nodes */
  size_t reply_len;
  /* total size of buf nodes */
  size_t reply_bytes;
  /* bytes sent to the client for current buffer (main buffer / node of reply
   * list) */
  size_t sent_len;
};
}  // namespace in_memory
}  // namespace redis_simple
