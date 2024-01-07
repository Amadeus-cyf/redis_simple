#include "ae.h"

#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <ctime>
#include <unordered_map>

#include "event_loop/ae_kqueue.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace ae {
AeEventLoop::AeEventLoop(AeKqueue* kq)
    : file_events_(std::vector<AeFileEvent*>(eventSize)),
      ae_api_state_(kq),
      max_fd_(-1) {}

AeEventLoop* AeEventLoop::InitEventLoop() {
  AeKqueue* kq = AeKqueue::AeApiCreate(eventSize);
  return new AeEventLoop(kq);
}

void AeEventLoop::AeMain() {
  while (true) {
    AeProcessEvents();
  }
}

int AeEventLoop::AeWait(int fd, int mask, long timeout) const {
  int nfds = 1;
  struct pollfd pfds[1];
  pfds[0].fd = fd;
  if (mask & AeFlags::aeReadable) {
    pfds[0].events = POLLIN;
  } else if (mask & AeFlags::aeWritable) {
    pfds[0].events = POLLOUT;
  }
  int r = poll(pfds, nfds, timeout);
  if (r < 0) {
    perror("Poll Error: ");
  }
  return r;
}

AeStatus AeEventLoop::AeCreateFileEvent(int fd, AeFileEvent* fe) {
  printf("create events for fd = %d, mask = %d\n", fd, fe->GetMask());
  if (fe == nullptr) {
    return aeErr;
  }
  if (fd < 0 || fd >= eventSize) {
    printf("file descriptor out of range");
    return aeErr;
  }
  if (int r = ae_api_state_->AeApiAddEvent(fd, fe->GetMask()); r < 0) {
    /* free the event if failed to add */
    delete fe;
    return aeErr;
  }
  if (file_events_[fd] == nullptr) {
    printf("add new event\n");
    max_fd_ = std::max(max_fd_, fd);
  } else {
    fe->Merge(file_events_[fd]);
    delete file_events_[fd];
  }
  file_events_[fd] = fe;
  return aeOK;
}

AeStatus AeEventLoop::AeDeleteFileEvent(int fd, int mask) {
  if (ae_api_state_->AeApiDelEvent(fd, mask) < 0) {
    printf(
        "fail to delete the file event of file descriptor %d with errno: "
        "%d\n",
        fd, errno);
    return aeErr;
  }
  printf("delete file event success for file descriptor = %d, mask = %d\n", fd,
         mask);
  AeFileEvent* fe = file_events_[fd];
  if (fe->GetMask() == mask) {
    file_events_[fd] = nullptr;
    delete fe;
    fe = nullptr;
  } else {
    int m = fe->GetMask();
    m &= ~mask;
    fe->SetMask(m);
  }
  return aeOK;
}

void AeEventLoop::AeCreateTimeEvent(AeTimeEvent* te) {
  if (!time_event_head_) {
    time_event_head_ = te;
    return;
  }
  te->SetNext(te);
  time_event_head_ = te;
}

void AeEventLoop::AeProcessEvents() {
  ProcessFileEvents();
  ProcessTimeEvents();
}

void AeEventLoop::ProcessFileEvents() {
  if (max_fd_ == -1) {
    return;
  }
  struct timespec tspec;
  tspec.tv_sec = 1;
  tspec.tv_nsec = 0;
  const std::unordered_map<int, int>& fdToMask =
      ae_api_state_->AeApiPoll(&tspec);
  for (const auto& it : fdToMask) {
    int fd = it.first, mask = it.second;
    const AeFileEvent* fe = file_events_[fd];
    int inverted = fe->GetMask() & AeFlags::aeBarrier;
    bool fired = false;
    if (!inverted && (mask & fe->GetMask() & AeFlags::aeReadable) &&
        fe->HasRFileProc()) {
      fe->CallReadProc(this, fd, mask);
      fired = true;
    }
    if ((mask & fe->GetMask() & AeFlags::aeWritable) && fe->HasWFileProc() &&
        (!fired || fe->IsRWProcDiff())) {
      fe->CallWriteProc(this, fd, mask);
      fired = true;
    }
    if (inverted && (mask & fe->GetMask() & AeFlags::aeReadable) &&
        fe->HasRFileProc() && (!fired || fe->IsRWProcDiff())) {
      fe->CallReadProc(this, fd, mask);
    }
  }
}

void AeEventLoop::ProcessTimeEvents() const {
  AeTimeEvent* te = time_event_head_;
  while (te) {
    long long id = te->Id();
    if (id == AeFlags::aeDeleteEventId) {
      AeTimeEvent *prev = te->Prev(), *next = te->Next();
      if (prev != nullptr) {
        prev->SetNext(next);
      } else {
        time_event_head_ = next;
      }
      if (next != nullptr) {
        next->SetPrev(prev);
      }
      if (te->HasTimeFinalizeProc()) {
        te->CallTimeFinalizeProc();
      }
      delete te;
      te = next;
    } else {
      int64_t now = utils::GetNowInMilliseconds();
      if (te->When() <= now) {
        int ret = te->CallTimeProc();
        if (ret == AeFlags::aeNoMore) {
          te->SetId(AeFlags::aeDeleteEventId);
        } else {
          te->SetWhen(now + ret * 1000);
        }
      }
      te = te->Next();
    }
  }
}

AeEventLoop::~AeEventLoop() {
  delete ae_api_state_;
  ae_api_state_ = nullptr;
  for (int i = 0; i < eventSize; ++i) {
    delete file_events_[i];
    file_events_[i] = nullptr;
  }
}
}  // namespace ae
}  // namespace redis_simple
