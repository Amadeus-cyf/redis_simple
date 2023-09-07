#include "reply_buffer.h"

namespace redis_simple {
namespace in_memory {
ReplyBuffer::ReplyBuffer()
    : buf_usable_size(4096), buf(new char[4096]), sent_len(0), bufpos(0) {}

size_t ReplyBuffer::addReplyToBufferOrList(const char* s, size_t len) {
  size_t reply = addReplyToBuffer(s, len);
  // printf("add reply %zu %zu\n", len, reply);
  len -= reply;
  size_t appended = 0;
  if (len) {
    appended = addReplyProtoToList(s + reply, len);
  }
  return reply + appended;
}

size_t ReplyBuffer::addReplyToBuffer(const char* s, size_t len) {
  if (reply_len > 0) {
    return 0;
  }
  size_t available = buf_usable_size - bufpos;
  size_t added_reply = len < available ? len : available;
  memcpy(buf + bufpos, s, added_reply);
  bufpos += added_reply;
  return added_reply;
}

size_t ReplyBuffer::addReplyProtoToList(const char* c, size_t len) {
  // printf("add reply to list %zu\n", len);
  if (!reply) {
    reply = BufNode::create(len);
    memcpy(reply->buf, c, len);
    reply->used = len;
    reply_bytes += reply->len;
    reply_len = 1;
    reply_tail = reply;
    printf("create reply head node\n");
    return len;
  }

  size_t available = reply_tail->len - reply_tail->used;
  size_t copy = len < available ? len : available;
  memcpy(reply_tail->buf + reply_tail->used, c, copy);
  reply_tail->used += copy;
  len -= copy, c += copy;

  if (len) {
    BufNode* new_node = BufNode::create(len);
    memcpy(new_node->buf, c, len);
    new_node->used += len;
    ++reply_len;
    reply_bytes += new_node->len;
    reply_tail->next = new_node;
    reply_tail = new_node;
  }
  return len + copy;
}

std::vector<std::pair<char*, size_t>> ReplyBuffer::getMemvec() {
  std::vector<std::pair<char*, size_t>> mem_vec;
  if (bufpos > 0) {
    mem_vec.push_back({buf + sent_len, bufpos - sent_len});
  }
  size_t offset = bufpos > 0 ? 0 : sent_len;
  BufNode *n = reply, *prev = nullptr;
  while (n) {
    if (n->used == 0) {
      reply_bytes -= n->len;
      prev ? prev->next = n->next : reply = n->next;
      if (!n->next) {
        reply_tail = prev ? prev : reply;
      }
      --reply_len;
      n->next = nullptr;
      delete n;
      offset = 0;
      continue;
    }
    mem_vec.push_back({n->buf + offset, n->used - offset});
    prev = n, n = n->next;
    offset = 0;
  }
  return mem_vec;
}

void ReplyBuffer::clearBuf() {
  memset(buf, bufpos, '\0');
  bufpos = 0;
  sent_len = 0;
}

void ReplyBuffer::writeProcessed(size_t nwritten) {
  reply ? _writevProcessed(nwritten) : _writeProcessed(nwritten);
}

void ReplyBuffer::_writeProcessed(size_t nwritten) {
  sent_len += nwritten;
  if (sent_len >= bufpos) {
    clearBuf();
  }
}

void ReplyBuffer::_writevProcessed(size_t nwritten) {
  printf("writevProcessed: nwritten %zu\n", nwritten);
  if (bufpos > 0) {
    nwritten -= (bufpos - sent_len);
    if (nwritten >= 0) {
      clearBuf();
    } else {
      sent_len += nwritten;
    }
  }
  printf("nwritten after processing main buffer %zu\n", nwritten);
  while (nwritten > 0) {
    if (nwritten < reply->used) {
      sent_len = nwritten;
      break;
    }
    nwritten -= (reply->used);
    sent_len = 0;
    reply_bytes -= reply->len;
    --reply_len;
    BufNode* n = reply;
    reply = reply->next;
    delete n;
  }
  if (!reply || !reply->next) {
    reply_tail = reply;
  }
}

ReplyBuffer::~ReplyBuffer() {
  if (buf) {
    delete[] buf;
    buf = nullptr;
  }
  while (reply) {
    BufNode* n = reply;
    reply = reply->next;
    n->next = nullptr;
    delete n;
  }
  reply_tail = nullptr;
}
}  // namespace in_memory
}  // namespace redis_simple
