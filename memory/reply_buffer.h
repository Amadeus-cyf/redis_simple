#pragma once

#include <vector>

#include "buf_node.h"

namespace redis_simple {
namespace in_memory {
class ReplyBuffer {
 public:
  ReplyBuffer();
  const char* getUnsentBuffer() { return buf + sent_len; }
  size_t getUnsentBufferLength() { return bufpos - sent_len; }
  size_t getSentLen() { return sent_len; };
  size_t getReplyLen() { return reply_len; };
  size_t getReplyBytes() { return reply_bytes; }
  size_t getBufPos() { return bufpos; };
  BufNode* getReplyHead() { return reply; }
  BufNode* getReplyTail() { return reply_tail; }
  size_t addReplyToBufferOrList(const char* s, size_t len);
  void writeProcessed(size_t nwritten);
  std::vector<std::pair<char*, size_t>> getMemvec();
  void clearBuf();
  bool isEmpty() { return bufpos == 0 && reply_len == 0; }
  ~ReplyBuffer();

 private:
  size_t addReplyToBuffer(const char* s, size_t len);
  size_t addReplyProtoToList(const char* c, size_t len);
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
