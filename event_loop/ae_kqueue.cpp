#include "ae_kqueue.h"

#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "ae.h"

namespace redis_simple {
namespace ae {
KqueueEventApi::KqueueEventApi(int fd, int nevents_)
    : kqueue_fd_(fd), nevents_(nevents_), events_(nevents_) {}

KqueueEventApi* KqueueEventApi::Create(int nevents_) {
  int kqueue_fd_ = kqueue();
  if (kqueue_fd_ < 0) {
    return nullptr;
  }
  return new KqueueEventApi(kqueue_fd_, nevents_);
}

int KqueueEventApi::AddEvent(int fd, int mask) const {
  RS_LOG_DEBUG("add api event for fd = %d, mask = %d\n", fd, mask);
  struct kevent ke;
  if (mask & EventFlag::kReadable) {
    RS_LOG_DEBUG("kqueue add read event\n");
    EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      RS_LOG_DEBUG("kevent add read event failed with errno: %d\n", errno);
      return -1;
    }
  }
  if (mask & EventFlag::kWritable) {
    RS_LOG_DEBUG("kqueue add write event\n");
    EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      RS_LOG_DEBUG("kevent add write event failed with errno: %d\n", errno);
      return -1;
    }
  }
  RS_LOG_DEBUG("add event success\n");
  return 0;
}

int KqueueEventApi::DeleteEvent(int fd, int mask) const {
  struct kevent ke;
  if (mask & EventFlag::kReadable) {
    EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      RS_LOG_DEBUG("kevent delete read event failed: %s\n",
                   std::strerror(errno));
      return -1;
    }
  }
  if (mask & EventFlag::kWritable) {
    EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      RS_LOG_DEBUG("kevent delete write event failed: %s\n",
                   std::strerror(errno));
      return -1;
    }
  }
  return 0;
}

std::unordered_map<int, int> KqueueEventApi::Poll(
    struct timespec* timeout_spec) const {
  std::unordered_map<int, int> fd_to_mask_map;
  int numevents =
      kevent(kqueue_fd_, nullptr, 0, events_.data(), nevents_, timeout_spec);
  if (numevents < 0) {
    RS_LOG_DEBUG("kevent poll failed: %s\n", std::strerror(errno));
    return fd_to_mask_map;
  }
  RS_LOG_DEBUG("kqueue: poll %d fds from kqueue\n", numevents);
  for (int i = 0; i < numevents; i++) {
    if (events_[i].filter == EVFILT_READ) {
      RS_LOG_DEBUG("kqueue: poll read\n");
      fd_to_mask_map[events_[i].ident] |= EventFlag::kReadable;
    }
    if (events_[i].filter == EVFILT_WRITE) {
      RS_LOG_DEBUG("kqueue: poll write\n");
      fd_to_mask_map[events_[i].ident] |= EventFlag::kWritable;
    }
  }
  return fd_to_mask_map;
}

KqueueEventApi::~KqueueEventApi() { close(kqueue_fd_); }
}  // namespace ae
}  // namespace redis_simple
