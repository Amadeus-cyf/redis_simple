#include "reply_buffer.h"

namespace redis_simple {
namespace in_memory {
ReplyBuffer::ReplyBuffer()
    : buf_usable_size_(defaultBufferSize),
      buf_(new char[defaultBufferSize]),
      sent_len_(0),
      buf_pos_(0) {}

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
  /* if reply list is used, store to the reply list first before storing it into
   * the main buffer */
  if (reply_len_ > 0) {
    return 0;
  }
  size_t available = buf_usable_size_ - buf_pos_;
  size_t nwritten = len < available ? len : available;
  memcpy(buf_ + buf_pos_, s, nwritten);
  buf_pos_ += nwritten;
  return nwritten;
}

/*
 * Add buffer to the reply list.
 */
size_t ReplyBuffer::AddReplyProtoToList(const char* c, size_t len) {
  /* if the reply list is not created yet, initialized the reply list through
   * creating a head node */
  if (!reply_head_) {
    BufNode* node = CreateReplyNode(c, len);
    AddNodeToReplyList(node);
  } else {
    /* store the buffer into the usable memory first */
    size_t available = reply_tail_->len_ - reply_tail_->used_;
    size_t nwritten = AppendToReplyNode(reply_tail_, c, len);
    size_t remain = len - nwritten;
    c += nwritten;
    /* If the usable memory is less than the buffer size, create a new node to
     * store the remaining buffer */
    if (len) {
      AddNodeToReplyList(CreateReplyNode(c, remain));
    }
  }
  return len;
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
  BufNode *n = reply_head_, *prev = nullptr;
  while (n) {
    /* if the reply list node has 0 used bytes, delete the node from the list */
    if (n->used_ == 0) {
      n = DeleteNodeFromReplyList(n, prev);
      offset = 0;
      continue;
    }
    /* push the list node memory and sent offset info into the result list */
    mem_vec.push_back({n->buf_ + offset, n->used_ - offset});
    prev = n;
    n = n->next_;
    offset = 0;
  }
  return mem_vec;
}

ReplyBuffer::~ReplyBuffer() {
  /* free main buffer */
  if (buf_) {
    delete[] buf_;
    buf_ = nullptr;
  }
  /* free the reply list */
  while (reply_head_) {
    reply_head_ = DeleteNodeFromReplyList(reply_head_, nullptr);
  }
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
  printf("clear processed: nwritten %zu\n", nwritten);
  /* clear main buffer first */
  size_t processed = ClearBufferProcessed(nwritten);
  /* clear the reply list if there are bytes remained */
  if (processed < nwritten) {
    ClearListProcessed(nwritten - processed);
  }
}

/*
 * Remove the processed part in the main buffer.
 */
size_t ReplyBuffer::ClearBufferProcessed(size_t nwritten) {
  if (buf_pos_ > 0) {
    size_t remain = buf_pos_ - sent_len_;
    nwritten = std::min(nwritten, remain);
    sent_len_ += nwritten;
    if (sent_len_ >= buf_pos_) {
      ClearBuffer();
    }
    return nwritten;
  }
  return 0;
}

/*
 * Remove the processed part in the reply list. The function assumes the main
 * buffer memory has already been cleared. Should call ClearBufferProcessed
 * before calling this function
 *
 */
void ReplyBuffer::ClearListProcessed(size_t nwritten) {
  printf("nwritten remained after processing main buffer %zu %zu\n", nwritten,
         reply_bytes_);
  /* if there are still written bytes remained, clear the reply list */
  while (nwritten > 0 && reply_head_) {
    if (nwritten < reply_head_->used_) {
      /* reach the last processed node, set sent len */
      sent_len_ = nwritten;
      break;
    }
    /* delete the node from the list */
    sent_len_ = 0;
    nwritten -= (reply_head_->used_);
    reply_head_ = DeleteNodeFromReplyList(reply_head_, nullptr);
  }
}

/*
 * Add the reply node to the reply list
 */
void ReplyBuffer::AddNodeToReplyList(BufNode* node) {
  ++reply_len_;
  reply_bytes_ += node->len_;
  if (!reply_head_) {
    reply_head_ = node;
  } else {
    reply_tail_->next_ = node;
  }
  reply_tail_ = node;
}

/*
 * Create a new list node.
 */
BufNode* ReplyBuffer::CreateReplyNode(const char* buffer, size_t len) {
  BufNode* node = BufNode::Create(len);
  AppendToReplyNode(node, buffer, len);
  return node;
}

/*
 * Delete the node from the reply list and return the next node.
 */
BufNode* ReplyBuffer::DeleteNodeFromReplyList(BufNode* node, BufNode* prev) {
  reply_bytes_ -= node->len_;
  --reply_len_;
  BufNode* next = node->next_;
  if (prev) {
    prev->next_ = next;
  } else {
    reply_head_ = next;
  }
  if (!next) reply_tail_ = prev;
  delete node;
  return next;
}

/*
 * Copy the buffer to the reply node. Return the actual number of bytes copied.
 */
size_t ReplyBuffer::AppendToReplyNode(BufNode* node, const char* buffer,
                                      size_t len) {
  size_t nwritten = std::min(len, node->len_ - node->used_);
  memcpy(node->buf_ + node->used_, buffer, nwritten);
  node->used_ += nwritten;
  return nwritten;
}
}  // namespace in_memory
}  // namespace redis_simple
