#include "ae.h"

#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <unordered_map>

#include "event_loop/ae_kqueue.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace ae {
int WaitForEvent(int fd, int mask, long timeout) {
  int fd_count = 1;
  struct pollfd poll_fds[1] = {};
  poll_fds[0].fd = fd;
  if (mask & EventFlag::kReadable) {
    poll_fds[0].events |= POLLIN;
  }
  if (mask & EventFlag::kWritable) {
    poll_fds[0].events |= POLLOUT;
  }
  int r = poll(poll_fds, fd_count, timeout);
  if (r < 0) {
    RS_LOG_DEBUG("poll error: %s\n", std::strerror(errno));
    return r;
  }
  if (r == 0) {
    // The file descriptor is not ready for the requested operation.
    return r;
  }
  int result_mask = 0;
  if (poll_fds[0].revents & POLL_IN) {
    result_mask |= EventFlag::kReadable;
  }
  if ((poll_fds[0].revents & POLL_OUT) || (poll_fds[0].revents & POLL_HUP) ||
      (poll_fds[0].revents & POLL_ERR)) {
    result_mask |= EventFlag::kWritable;
  }
  return result_mask;
}

EventLoop::EventLoop(KqueueEventApi* kq)
    : file_events_(std::vector<FileEvent*>(kEventSize)),
      time_event_head_(nullptr),
      event_api_(kq),
      max_fd_(-1) {}

EventLoop* EventLoop::Create() {
  KqueueEventApi* kq = KqueueEventApi::Create(kEventSize);
  return new EventLoop(kq);
}

void EventLoop::Run() {
  while (true) {
    ProcessEvents();
  }
}

EventLoopStatus EventLoop::CreateFileEvent(int fd, FileEvent* file_event) {
  RS_LOG_DEBUG("create events for fd = %d, mask = %d\n", fd,
               file_event->Mask());
  if (file_event == nullptr) {
    return kError;
  }
  if (fd < 0 || fd >= kEventSize) {
    RS_LOG_DEBUG("file descriptor out of range");
    return kError;
  }
  if (event_api_->AddEvent(fd, file_event->Mask()) < 0) {
    delete file_event;
    return kError;
  }
  if (file_events_[fd] == nullptr) {
    RS_LOG_DEBUG("add new event\n");
    max_fd_ = std::max(max_fd_, fd);
  } else {
    // A descriptor stores one FileEvent, so merge separately registered
    // read/write callbacks before replacing the old event.
    file_event->Merge(file_events_[fd]);
    delete file_events_[fd];
  }
  file_events_[fd] = file_event;
  return kOk;
}

EventLoopStatus EventLoop::DeleteFileEvent(int fd, int mask) {
  if (event_api_->DeleteEvent(fd, mask) < 0) {
    RS_LOG_DEBUG(
        "fail to delete the file event of file descriptor %d with errno: "
        "%d\n",
        fd, errno);
    return kError;
  }
  RS_LOG_DEBUG(
      "delete file event success for file descriptor = %d, mask = %d\n", fd,
      mask);
  FileEvent* file_event = file_events_[fd];
  if (file_event->Mask() == mask) {
    file_events_[fd] = nullptr;
    delete file_event;
    file_event = nullptr;
  } else {
    int m = file_event->Mask();
    m &= ~mask;
    file_event->SetMask(m);
  }
  return kOk;
}

void EventLoop::CreateTimeEvent(TimeEvent* time_event) {
  if (!time_event_head_) {
    time_event_head_ = time_event;
    return;
  }
  time_event->SetNext(time_event_head_);
  time_event_head_ = time_event;
}

void EventLoop::ProcessEvents() {
  ProcessFileEvents();
  ProcessTimeEvents();
}

void EventLoop::ProcessFileEvents() {
  if (max_fd_ == -1) {
    return;
  }
  struct timespec timeout_spec;
  timeout_spec.tv_sec = 1;
  timeout_spec.tv_nsec = 0;
  const std::unordered_map<int, int>& fd_to_mask =
      event_api_->Poll(&timeout_spec);
  for (const auto& it : fd_to_mask) {
    int fd = it.first, mask = it.second;
    const FileEvent* file_event = file_events_[fd];
    int inverted = file_event->Mask() & EventFlag::kBarrier;
    bool fired = false;
    // Normal order is read-before-write; kBarrier lets a pending write flush
    // before another read queues more output on the same descriptor.
    if (!inverted && (mask & file_event->Mask() & EventFlag::kReadable) &&
        file_event->HasReadCallback()) {
      file_event->CallReadCallback(this, fd, mask);
      fired = true;
    }
    if ((mask & file_event->Mask() & EventFlag::kWritable) &&
        file_event->HasWriteCallback() &&
        (!fired || file_event->HasSeparateReadWriteCallbacks())) {
      file_event->CallWriteCallback(this, fd, mask);
      fired = true;
    }
    if (inverted && (mask & file_event->Mask() & EventFlag::kReadable) &&
        file_event->HasReadCallback() &&
        (!fired || file_event->HasSeparateReadWriteCallbacks())) {
      file_event->CallReadCallback(this, fd, mask);
    }
  }
}

void EventLoop::ProcessTimeEvents() const {
  TimeEvent* time_event = time_event_head_;
  while (time_event) {
    long long id = time_event->Id();
    if (id == EventFlag::kDeleteEventId) {
      TimeEvent *prev = time_event->Prev(), *next = time_event->Next();
      if (prev != nullptr) {
        prev->SetNext(next);
      } else {
        time_event_head_ = next;
      }
      if (next != nullptr) {
        next->SetPrev(prev);
      }
      if (time_event->HasFinalizeCallback()) {
        time_event->CallFinalizeCallback();
      }
      delete time_event;
      time_event = next;
    } else {
      int64_t now = utils::GetNowInMilliseconds();
      if (time_event->When() <= now) {
        int ret = time_event->CallTimeCallback();
        if (ret == EventFlag::kNoMore) {
          // Defer deletion so unlinking and finalization happen in one path.
          time_event->SetId(EventFlag::kDeleteEventId);
        } else {
          time_event->SetWhen(now + ret * 1000);
        }
      }
      time_event = time_event->Next();
    }
  }
}

EventLoop::~EventLoop() {
  delete event_api_;
  event_api_ = nullptr;
  for (int i = 0; i < kEventSize; ++i) {
    delete file_events_[i];
    file_events_[i] = nullptr;
  }
}
}  // namespace ae
}  // namespace redis_simple
