#include "event_loop/epoll_poller.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <ctime>
#include <limits>

#include "event_loop/loop.h"

namespace redis_simple::event_loop {
namespace {
uint32_t ToEpollEvents(int mask) {
  uint32_t events = 0;
  if ((mask & EventFlag::kReadable) != 0) {
    events |= EPOLLIN;
  }
  if ((mask & EventFlag::kWritable) != 0) {
    events |= EPOLLOUT;
  }
  return events;
}

int ToTimeoutMilliseconds(const struct timespec* const timeout_spec) {
  if (timeout_spec == nullptr) {
    return -1;
  }
  constexpr int64_t kMillisecondsPerSecond = 1000;
  constexpr int64_t kNanosecondsPerMillisecond = 1000000;
  const int64_t seconds = timeout_spec->tv_sec;
  const int64_t nanoseconds = timeout_spec->tv_nsec;
  if (seconds > std::numeric_limits<int>::max() / kMillisecondsPerSecond) {
    return std::numeric_limits<int>::max();
  }
  const int64_t milliseconds = seconds * kMillisecondsPerSecond +
                               nanoseconds / kNanosecondsPerMillisecond;
  return milliseconds > std::numeric_limits<int>::max()
             ? std::numeric_limits<int>::max()
             : static_cast<int>(milliseconds);
}
}  // namespace

EpollPoller::EpollPoller(int fd, int nevents)
    : epoll_fd_(fd), nevents_(nevents), events_(nevents) {}

std::unique_ptr<EpollPoller> EpollPoller::Create(int nevents) {
  const int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd < 0) {
    RS_LOG_DEBUG("epoll_create1 failed: %s\n", std::strerror(errno));
    return nullptr;
  }
  return std::unique_ptr<EpollPoller>(new EpollPoller(epoll_fd, nevents));
}

int EpollPoller::AddEvent(int fd, int mask) const {
  const auto existing_mask = masks_.find(fd);
  const int new_mask =
      existing_mask == masks_.end() ? mask : (existing_mask->second | mask);

  struct epoll_event event {};
  event.events = ToEpollEvents(new_mask);
  event.data.fd = fd;
  const int operation =
      existing_mask == masks_.end() ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
  if (epoll_ctl(epoll_fd_, operation, fd, &event) < 0) {
    RS_LOG_DEBUG("epoll add event failed: %s\n", std::strerror(errno));
    return -1;
  }
  masks_[fd] = new_mask;
  return 0;
}

int EpollPoller::DeleteEvent(int fd, int mask) const {
  const auto existing_mask = masks_.find(fd);
  if (existing_mask == masks_.end()) {
    return -1;
  }
  const int remaining_mask = existing_mask->second & ~mask;
  if (remaining_mask == 0) {
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
      RS_LOG_DEBUG("epoll delete event failed: %s\n", std::strerror(errno));
      return -1;
    }
    masks_.erase(existing_mask);
    return 0;
  }

  struct epoll_event event {};
  event.events = ToEpollEvents(remaining_mask);
  event.data.fd = fd;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0) {
    RS_LOG_DEBUG("epoll modify event failed: %s\n", std::strerror(errno));
    return -1;
  }
  masks_[fd] = remaining_mask;
  return 0;
}

std::unordered_map<int, int> EpollPoller::Poll(
    struct timespec* timeout_spec) const {
  std::unordered_map<int, int> fd_to_mask_map;
  const int numevents = epoll_wait(epoll_fd_, events_.data(), nevents_,
                                   ToTimeoutMilliseconds(timeout_spec));
  if (numevents < 0) {
    RS_LOG_DEBUG("epoll_wait failed: %s\n", std::strerror(errno));
    return fd_to_mask_map;
  }
  for (int i = 0; i < numevents; ++i) {
    const int fd = events_[i].data.fd;
    const uint32_t events = events_[i].events;
    if ((events & (EPOLLIN | EPOLLERR | EPOLLHUP)) != 0) {
      fd_to_mask_map[fd] |= EventFlag::kReadable;
    }
    if ((events & (EPOLLOUT | EPOLLERR | EPOLLHUP)) != 0) {
      fd_to_mask_map[fd] |= EventFlag::kWritable;
    }
  }
  return fd_to_mask_map;
}

EpollPoller::~EpollPoller() { close(epoll_fd_); }
}  // namespace redis_simple::event_loop
