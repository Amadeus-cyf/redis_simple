#include "ae_kqueue.h"

#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "ae.h"

namespace redis_simple {
namespace ae {
AeKqueue::AeKqueue(int fd, int nevents_)
    : kqueue_fd_(fd), nevents_(nevents_), events_(nevents_) {}

AeKqueue* AeKqueue::AeApiCreate(int nevents_) {
  int kqueue_fd_ = kqueue();
  if (kqueue_fd_ < 0) {
    return nullptr;
  }
  return new AeKqueue(kqueue_fd_, nevents_);
}

int AeKqueue::AeApiAddEvent(int fd, int mask) const {
  RS_LOG_DEBUG("add api event for fd = %d, mask = %d\n", fd, mask);
  struct kevent ke;
  if (mask & AeFlags::aeReadable) {
    RS_LOG_DEBUG("kqueue add read event\n");
    EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      RS_LOG_DEBUG("kevent add read event failed with errno: %d\n", errno);
      return -1;
    }
  }
  if (mask & AeFlags::aeWritable) {
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

int AeKqueue::AeApiDelEvent(int fd, int mask) const {
  struct kevent ke;
  if (mask & AeFlags::aeReadable) {
    EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      RS_LOG_DEBUG("kevent delete read event failed: %s\n",
                   std::strerror(errno));
      return -1;
    }
  }
  if (mask & AeFlags::aeWritable) {
    EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      RS_LOG_DEBUG("kevent delete write event failed: %s\n",
                   std::strerror(errno));
      return -1;
    }
  }
  return 0;
}

std::unordered_map<int, int> AeKqueue::AeApiPoll(struct timespec* tspec) const {
  std::unordered_map<int, int> fdToMaskMap;
  int numevents =
      kevent(kqueue_fd_, nullptr, 0, events_.data(), nevents_, tspec);
  if (numevents < 0) {
    RS_LOG_DEBUG("kevent poll failed: %s\n", std::strerror(errno));
    return fdToMaskMap;
  }
  RS_LOG_DEBUG("kqueue: poll %d fds from kqueue\n", numevents);
  for (int i = 0; i < numevents; i++) {
    if (events_[i].filter == EVFILT_READ) {
      RS_LOG_DEBUG("kqueue: poll read\n");
      fdToMaskMap[events_[i].ident] |= AeFlags::aeReadable;
    }
    if (events_[i].filter == EVFILT_WRITE) {
      RS_LOG_DEBUG("kqueue: poll write\n");
      fdToMaskMap[events_[i].ident] |= AeFlags::aeWritable;
    }
  }
  return fdToMaskMap;
}

AeKqueue::~AeKqueue() { close(kqueue_fd_); }
}  // namespace ae
}  // namespace redis_simple
