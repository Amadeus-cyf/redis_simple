#include "reply_buffer.h"

namespace redis_simple {
namespace in_memory {
ReplyBuffer::ReplyBuffer()
    : buf_usable_size_(kDefaultBufferSize),
      buf_(new char[kDefaultBufferSize]),
      sent_len_(0),
      buf_pos_(0) {}

size_t ReplyBuffer::AddReplyToBufferOrList(const char* s, size_t len) {
  if (len == 0) {
    return 0;
  }
  size_t nwritten = AddReplyToBuffer(s, len);
  if (nwritten < len) {
    nwritten += AddReplyProtoToList(s + nwritten, len - nwritten);
  }
  return nwritten;
}

size_t ReplyBuffer::AddReplyToBuffer(const char* s, size_t len) {
  // Preserve write order: once overflow nodes exist, new bytes must also go to
  // the list until earlier list data is drained.
  if (reply_len_ == 0) {
    size_t available = buf_usable_size_ - buf_pos_;
    size_t nwritten = len < available ? len : available;
    std::memcpy(buf_ + buf_pos_, s, nwritten);
    buf_pos_ += nwritten;
    return nwritten;
  }
  return 0;
}

size_t ReplyBuffer::AddReplyProtoToList(const char* c, size_t len) {
  size_t appended = 0;
  if (reply_head_) {
    appended = AppendToReplyNode(reply_tail_, c, len);
  }
  if (appended < len) {
    BufNode* node = CreateReplyNode(c + appended, len - appended);
    AddNodeToReplyList(node);
  }
  return len;
}

std::vector<std::pair<char*, size_t>> ReplyBuffer::Memvec() {
  std::vector<std::pair<char*, size_t>> mem_vec;
  if (buf_pos_ > 0) {
    mem_vec.push_back({buf_ + sent_len_, buf_pos_ - sent_len_});
  }
  size_t offset = buf_pos_ > 0 ? 0 : sent_len_;
  BufNode *n = reply_head_, *prev = nullptr;
  while (n) {
    if (n->used_ == 0) {
      BufNode* next = n->next_;
      DeleteNodeFromReplyList(n, prev);
      n = next;
      offset = 0;
      continue;
    }
    mem_vec.push_back({n->buf_ + offset, n->used_ - offset});
    prev = n, n = n->next_;
    offset = 0;
  }
  return mem_vec;
}

ReplyBuffer::~ReplyBuffer() {
  if (buf_) {
    delete[] buf_;
    buf_ = nullptr;
  }
  while (reply_head_) {
    DeleteNodeFromReplyList(reply_head_, nullptr);
  }
}

void ReplyBuffer::ClearBuffer() {
  std::memset(buf_, '\0', buf_pos_);
  buf_pos_ = 0;
  sent_len_ = 0;
}

void ReplyBuffer::ClearProcessed(size_t nwritten) {
  RS_LOG_DEBUG("clear processed: nwritten %zu\n", nwritten);
  size_t processed = ClearBufferProcessed(nwritten);
  if (processed < nwritten) {
    ClearListProcessed(nwritten - processed);
  }
}

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

void ReplyBuffer::ClearListProcessed(size_t nwritten) {
  RS_LOG_DEBUG("nwritten remained after processing main buffer %zu %zu\n",
               nwritten, reply_bytes_);
  while (nwritten > 0 && reply_head_) {
    if (nwritten < reply_head_->used_) {
      sent_len_ = nwritten;
      break;
    }
    sent_len_ = 0;
    nwritten -= reply_head_->used_;
    DeleteNodeFromReplyList(reply_head_, nullptr);
  }
}

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

void ReplyBuffer::DeleteNodeFromReplyList(BufNode* node, BufNode* prev) {
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
  node = nullptr;
}

BufNode* ReplyBuffer::CreateReplyNode(const char* buffer, size_t len) {
  BufNode* node = BufNode::Create(len);
  AppendToReplyNode(node, buffer, len);
  return node;
}

size_t ReplyBuffer::AppendToReplyNode(BufNode* node, const char* buffer,
                                      size_t len) {
  size_t nwritten = std::min(len, node->len_ - node->used_);
  std::memcpy(node->buf_ + node->used_, buffer, nwritten);
  node->used_ += nwritten;
  return nwritten;
}
}  // namespace in_memory
}  // namespace redis_simple
