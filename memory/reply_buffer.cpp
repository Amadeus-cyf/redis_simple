#include "reply_buffer.h"

namespace redis_simple::in_memory {
ReplyBuffer::ReplyBuffer()
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays): contiguous byte buffer
    : buf_(std::make_unique<char[]>(kDefaultBufferSize)),
      size_(0),
      buf_usable_size_(kDefaultBufferSize),
      sent_(0) {}

size_t ReplyBuffer::Append(const char* s, size_t len) {
  if (len == 0) {
    return 0;
  }
  size_t nwritten = AppendToBuffer(s, len);
  if (nwritten < len) {
    nwritten += AppendToList(s + nwritten, len - nwritten);
  }
  return nwritten;
}

size_t ReplyBuffer::AppendToBuffer(const char* s, size_t len) {
  // Preserve write order: once overflow nodes exist, new bytes must also go to
  // the list until earlier list data is drained.
  if (node_count_ == 0) {
    size_t available = buf_usable_size_ - size_;
    size_t nwritten = len < available ? len : available;
    std::memcpy(buf_.get() + size_, s, nwritten);
    size_ += nwritten;
    return nwritten;
  }
  return 0;
}

size_t ReplyBuffer::AppendToList(const char* c, size_t len) {
  size_t appended = 0;
  if (reply_head_ != nullptr) {
    appended = AppendToNode(reply_tail_, c, len);
  }
  if (appended < len) {
    auto node = NewNode(c + appended, len - appended);
    PushNode(std::move(node));
  }
  return len;
}

std::vector<std::pair<char*, size_t>> ReplyBuffer::Blocks() {
  std::vector<std::pair<char*, size_t>> mem_vec;
  if (size_ > 0) {
    mem_vec.emplace_back(buf_.get() + sent_, size_ - sent_);
  }
  size_t offset = size_ > 0 ? 0 : sent_;
  BufNode* n = reply_head_.get();
  BufNode* prev = nullptr;
  while (n != nullptr) {
    if (n->used_ == 0) {
      BufNode* next = n->next_.get();
      DeleteNode(n, prev);
      n = next;
      offset = 0;
      continue;
    }
    mem_vec.emplace_back(n->buf_.get() + offset, n->used_ - offset);
    prev = n, n = n->next_.get();
    offset = 0;
  }
  return mem_vec;
}

void ReplyBuffer::ClearBuffer() {
  std::memset(buf_.get(), '\0', size_);
  size_ = 0;
  sent_ = 0;
}

void ReplyBuffer::Consume(size_t nwritten) {
  RS_LOG_DEBUG("clear processed: nwritten %zu\n", nwritten);
  size_t processed = ConsumeBuffer(nwritten);
  if (processed < nwritten) {
    ConsumeList(nwritten - processed);
  }
}

size_t ReplyBuffer::ConsumeBuffer(size_t nwritten) {
  if (size_ > 0) {
    size_t remain = size_ - sent_;
    nwritten = std::min(nwritten, remain);
    sent_ += nwritten;
    if (sent_ >= size_) {
      ClearBuffer();
    }
    return nwritten;
  }
  return 0;
}

void ReplyBuffer::ConsumeList(size_t nwritten) {
  RS_LOG_DEBUG("nwritten remained after processing main buffer %zu %zu\n",
               nwritten, reply_bytes_);
  while (nwritten > 0 && (reply_head_ != nullptr)) {
    if (nwritten < reply_head_->used_) {
      sent_ = nwritten;
      break;
    }
    sent_ = 0;
    nwritten -= reply_head_->used_;
    DeleteNode(reply_head_.get(), nullptr);
  }
}

void ReplyBuffer::PushNode(std::unique_ptr<BufNode> node) {
  ++node_count_;
  reply_bytes_ += node->capacity_;
  BufNode* node_ptr = node.get();
  if (reply_head_ == nullptr) {
    reply_head_ = std::move(node);
  } else {
    reply_tail_->next_ = std::move(node);
  }
  reply_tail_ = node_ptr;
}

void ReplyBuffer::DeleteNode(BufNode* node, BufNode* prev) {
  reply_bytes_ -= node->capacity_;
  --node_count_;
  if (prev != nullptr) {
    auto owned_node = std::move(prev->next_);
    prev->next_ = std::move(owned_node->next_);
  } else {
    reply_head_ = std::move(reply_head_->next_);
  }
  if ((prev != nullptr && prev->next_ == nullptr) || reply_head_ == nullptr) {
    reply_tail_ = prev;
  }
}

std::unique_ptr<BufNode> ReplyBuffer::NewNode(const char* buffer, size_t len) {
  auto node = BufNode::Create(len);
  AppendToNode(node.get(), buffer, len);
  return node;
}

size_t ReplyBuffer::AppendToNode(BufNode* node, const char* buffer,
                                 size_t len) {
  size_t nwritten = std::min(len, node->capacity_ - node->used_);
  std::memcpy(node->buf_.get() + node->used_, buffer, nwritten);
  node->used_ += nwritten;
  return nwritten;
}
}  // namespace redis_simple::in_memory
