#include "ae_kqueue.h"

#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>

#include "ae.h"

namespace redis_simple {
namespace ae {
AeKqueue::AeKqueue(int fd, int nevents_) : kqueue_fd_(fd), nevents_(nevents_) {}

AeKqueue* AeKqueue::AeApiCreate(int nevents_) {
  int kqueue_fd_ = kqueue();
  if (kqueue_fd_ < 0) {
    return nullptr;
  }
  return new AeKqueue(kqueue_fd_, nevents_);
}

int AeKqueue::AeApiAddEvent(int fd, int mask) const {
  printf("add api event for fd = %d, mask = %d\n", fd, mask);
  struct kevent ke;
  if (mask & AeFlags::aeReadable) {
    printf("kqueue add read event\n");
    EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      printf("kevent add read event failed with errno: %d\n", errno);
      return -1;
    }
  }
  if (mask & AeFlags::aeWritable) {
    printf("kqueue add write event\n");
    EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      printf("kevent add write event failed with errno: %d\n", errno);
      return -1;
    }
  }
  printf("add event success\n");
  return 0;
}

int AeKqueue::AeApiDelEvent(int fd, int mask) const {
  struct kevent ke;
  if (mask & AeFlags::aeReadable) {
    EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      perror("kevent delete read event failed with errno");
      return -1;
    }
  }
  if (mask & AeFlags::aeWritable) {
    EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    if (kevent(kqueue_fd_, &ke, 1, nullptr, 0, nullptr) < 0) {
      perror("kevent delete write event failed with errno");
      return -1;
    }
  }
  return 0;
}

std::unordered_map<int, int> AeKqueue::AeApiPoll(struct timespec* tspec) const {
  struct kevent events[nevents_];
  std::unordered_map<int, int> fdToMaskMap;
  int retval = kevent(kqueue_fd_, nullptr, 0, events, nevents_, tspec);
  if (retval < 0) {
    perror("Error: ");
    printf("Errno %d\n", errno);
    return fdToMaskMap;
  }
  printf("kqueue: poll %d fds from kqueue\n", retval);
  for (int i = 0; i < retval; i++) {
    if (events[i].filter == EVFILT_READ) {
      printf("kqueue: poll read\n");
      fdToMaskMap[events[i].ident] |= AeFlags::aeReadable;
    }
    if (events[i].filter == EVFILT_WRITE) {
      printf("kqueue: poll write\n");
      fdToMaskMap[events[i].ident] |= AeFlags::aeWritable;
    }
  }
  return fdToMaskMap;
}

AeKqueue::~AeKqueue() { close(kqueue_fd_); }
}  // namespace ae
}  // namespace redis_simple
