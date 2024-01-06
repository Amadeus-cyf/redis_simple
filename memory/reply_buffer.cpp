#include "reply_buffer.h"

namespace redis_simple {
namespace in_memory {
ReplyBuffer::ReplyBuffer()
    : buf_usable_size_(4096), buf_(new char[4096]), sent_len_(0), buf_pos_(0) {}

/*
 * Add buffer to the main buffer or the reply list.
 */
size_t ReplyBuffer::AddReplyToBufferOrList(const char* s, size_t len) {
  if (len == 0) {
    return 0;
  }
  /* try to add to the main buffer first */
  size_t reply = AddReplyToBuffer(s, len);
  len -= reply;
  size_t appended = 0;
  /* Add the remaining part (if any) to the reply list */
  if (len) {
    appended = AddReplyProtoToList(s + reply, len);
  }
  return reply + appended;
}

/*
 * Add buffer to the main buffer.
 */
size_t ReplyBuffer::AddReplyToBuffer(const char* s, size_t len) {
  if (reply_len_ > 0) {
    return 0;
  }
  size_t available = buf_usable_size_ - buf_pos_;
  size_t added_reply = len < available ? len : available;
  memcpy(buf_ + buf_pos_, s, added_reply);
  buf_pos_ += added_reply;
  return added_reply;
}

/*
 * Add buffer to the reply list.
 */
size_t ReplyBuffer::AddReplyProtoToList(const char* c, size_t len) {
  if (!reply_) {
    reply_ = BufNode::Create(len);
    memcpy(reply_->buf_, c, len);
    reply_->used_ = len;
    reply_bytes_ += reply_->len_;
    reply_len_ = 1;
    reply_tail_ = reply_;
    printf("create reply head node\n");
    return len;
  }

  size_t available = reply_tail_->len_ - reply_tail_->used_;
  size_t copy = len < available ? len : available;
  memcpy(reply_tail_->buf_ + reply_tail_->used_, c, copy);
  reply_tail_->used_ += copy;
  len -= copy, c += copy;

  if (len) {
    BufNode* new_node = BufNode::Create(len);
    memcpy(new_node->buf_, c, len);
    new_node->used_ += len;
    ++reply_len_;
    reply_bytes_ += new_node->len_;
    reply_tail_->next_ = new_node;
    reply_tail_ = new_node;
  }
  return len + copy;
}

/*
 * Turn the reply buffer to a vector of buffer. Used for writev
 */
std::vector<std::pair<char*, size_t>> ReplyBuffer::Memvec() {
  std::vector<std::pair<char*, size_t>> mem_vec;
  if (buf_pos_ > 0) {
    mem_vec.push_back({buf_ + sent_len_, buf_pos_ - sent_len_});
  }
  size_t offset = buf_pos_ > 0 ? 0 : sent_len_;
  BufNode *n = reply_, *prev = nullptr;
  while (n) {
    if (n->used_ == 0) {
      reply_bytes_ -= n->len_;
      prev ? prev->next_ = n->next_ : reply_ = n->next_;
      if (!n->next_) {
        reply_tail_ = prev ? prev : reply_;
      }
      --reply_len_;
      n->next_ = nullptr;
      delete n;
      offset = 0;
      continue;
    }
    mem_vec.push_back({n->buf_ + offset, n->used_ - offset});
    prev = n, n = n->next_;
    offset = 0;
  }
  return mem_vec;
}

/*
 * Clear the main buffer.
 */
void ReplyBuffer::ClearBuffer() {
  memset(buf_, '\0', buf_pos_);
  buf_pos_ = 0;
  sent_len_ = 0;
}

/*
 * Remove the processed content.
 */
void ReplyBuffer::ClearProcessed(size_t nwritten) {
  reply_ ? ClearListProcessed(nwritten) : ClearBufferProcessed(nwritten);
}

/*
 * Remove the processed part in the main buffer.
 */
void ReplyBuffer::ClearBufferProcessed(size_t nwritten) {
  sent_len_ += nwritten;
  if (sent_len_ >= buf_pos_) {
    ClearBuffer();
  }
}

/*
 * Remove the processed part in the main buffer and the reply list.
 */
void ReplyBuffer::ClearListProcessed(size_t nwritten) {
  printf("writevProcessed: nwritten %zu\n", nwritten);
  if (buf_pos_ > 0) {
    nwritten -= (buf_pos_ - sent_len_);
    if (nwritten >= 0) {
      ClearBuffer();
    } else {
      sent_len_ += nwritten;
    }
  }
  printf("nwritten after processing main buf_fer %zu %zu\n", nwritten,
         reply_bytes_);
  while (nwritten > 0) {
    if (nwritten < reply_->used_) {
      sent_len_ = nwritten;
      break;
    }
    nwritten -= (reply_->used_);
    sent_len_ = 0;
    reply_bytes_ -= reply_->len_;
    --reply_len_;
    BufNode* n = reply_;
    reply_ = reply_->next_;
    delete n;
  }
  if (!reply_ || !reply_->next_) {
    reply_tail_ = reply_;
  }
}

ReplyBuffer::~ReplyBuffer() {
  if (buf_) {
    delete[] buf_;
    buf_ = nullptr;
  }
  while (reply_) {
    BufNode* n = reply_;
    reply_ = reply_->next_;
    n->next_ = nullptr;
    delete n;
  }
  reply_tail_ = nullptr;
}
}  // namespace in_memory
}  // namespace redis_simple
