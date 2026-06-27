#include "ae.h"

#include <poll.h>
#include <sys/time.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <limits>
#include <unordered_map>

#include "event_loop/ae_kqueue.h"
#include "utils/time_utils.h"

namespace redis_simple::ae {
void FileEvent::Merge(const FileEvent* file_event) {
  if (file_event == nullptr) {
    return;
  }
  AddMask(file_event->Mask());
  if (!HasReadCallback() && file_event->HasReadCallback()) {
    read_callback_ = file_event->read_callback_;
  }
  if (!HasWriteCallback() && file_event->HasWriteCallback()) {
    write_callback_ = file_event->write_callback_;
  }
  has_separate_read_write_callbacks_ =
      HasReadCallback() && HasWriteCallback() &&
      (has_separate_read_write_callbacks_ ||
       file_event->has_separate_read_write_callbacks_);
}

int WaitForEvent(int fd, int mask, long timeout) {
  int fd_count = 1;
  if (timeout > std::numeric_limits<int>::max()) {
    timeout = std::numeric_limits<int>::max();
  }
  struct pollfd poll_fds[1] = {};
  poll_fds[0].fd = fd;
  if ((mask & EventFlag::kReadable) != 0) {
    poll_fds[0].events |= POLLIN;
  }
  if ((mask & EventFlag::kWritable) != 0) {
    poll_fds[0].events |= POLLOUT;
  }
  int r = poll(poll_fds, fd_count, static_cast<int>(timeout));
  if (r < 0) {
    RS_LOG_DEBUG("poll error: %s\n", std::strerror(errno));
    return r;
  }
  if (r == 0) {
    // The file descriptor is not ready for the requested operation.
    return r;
  }
  int result_mask = 0;
  if ((poll_fds[0].revents & POLLIN) != 0) {
    result_mask |= EventFlag::kReadable;
  }
  if (((poll_fds[0].revents & POLLOUT) != 0) ||
      ((poll_fds[0].revents & POLLHUP) != 0) ||
      ((poll_fds[0].revents & POLLERR) != 0)) {
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
    return EventLoopStatus::kError;
  }
  if (fd < 0 || fd >= kEventSize) {
    RS_LOG_DEBUG("file descriptor out of range");
    return EventLoopStatus::kError;
  }
  if (event_api_->AddEvent(fd, file_event->Mask()) < 0) {
    delete file_event;
    return EventLoopStatus::kError;
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
  return EventLoopStatus::kOk;
}

EventLoopStatus EventLoop::DeleteFileEvent(int fd, int mask) {
  if (event_api_->DeleteEvent(fd, mask) < 0) {
    RS_LOG_DEBUG(
        "fail to delete the file event of file descriptor %d with errno: "
        "%d\n",
        fd, errno);
    return EventLoopStatus::kError;
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
  return EventLoopStatus::kOk;
}

void EventLoop::CreateTimeEvent(TimeEvent* time_event) {
  if (time_event_head_ == nullptr) {
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
    int fd = it.first;
    int mask = it.second;
    const FileEvent* file_event = file_events_[fd];
    int inverted = file_event->Mask() & EventFlag::kBarrier;
    bool fired = false;
    // Normal order is read-before-write; kBarrier lets a pending write flush
    // before another read queues more output on the same descriptor.
    if ((inverted == 0) &&
        ((mask & file_event->Mask() & EventFlag::kReadable) != 0) &&
        file_event->HasReadCallback()) {
      file_event->CallReadCallback(this, fd, mask);
      fired = true;
    }
    if (((mask & file_event->Mask() & EventFlag::kWritable) != 0) &&
        file_event->HasWriteCallback() &&
        (!fired || file_event->HasSeparateReadWriteCallbacks())) {
      file_event->CallWriteCallback(this, fd, mask);
      fired = true;
    }
    if ((inverted != 0) &&
        ((mask & file_event->Mask() & EventFlag::kReadable) != 0) &&
        file_event->HasReadCallback() &&
        (!fired || file_event->HasSeparateReadWriteCallbacks())) {
      file_event->CallReadCallback(this, fd, mask);
    }
  }
}

void EventLoop::ProcessTimeEvents() const {
  TimeEvent* time_event = time_event_head_;
  while (time_event != nullptr) {
    const auto id = static_cast<EventFlag>(time_event->Id());
    if (id == EventFlag::kDeleteEventId) {
      TimeEvent* prev = time_event->Prev();
      TimeEvent* next = time_event->Next();
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
          time_event->SetId(ToInt(EventFlag::kDeleteEventId));
        } else {
          time_event->SetWhen(now + (static_cast<int64_t>(ret * 1000)));
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
}  // namespace redis_simple::ae
