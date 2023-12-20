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
    : fileEvents(std::vector<AeFileEvent*>(EventsSize)),
      aeApiState(kq),
      max_fd(-1) {}

std::unique_ptr<AeEventLoop> AeEventLoop::InitEventLoop() {
  AeKqueue* kq = AeKqueue::AeApiCreate(EventsSize);
  return std::unique_ptr<AeEventLoop>(new AeEventLoop(kq));
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
    return ae_err;
  }
  if (fd < 0 || fd >= EventsSize) {
    throw("file descriptor out of range");
  }
  int mask = fe->GetMask();
  if (fileEvents[fd] == nullptr) {
    printf("add new event\n");
    max_fd = std::max(max_fd, fd);
  } else {
    fe->Merge(fileEvents[fd]);
    delete fileEvents[fd];
  }
  fileEvents[fd] = fe;
  return aeApiState->AeApiAddEvent(fd, mask) < 0 ? ae_err : ae_ok;
}

AeStatus AeEventLoop::AeDeleteFileEvent(int fd, int mask) {
  if (aeApiState->AeApiDelEvent(fd, mask) < 0) {
    printf(
        "fail to delete the file event of file descriptor %d with errno: "
        "%d\n",
        fd, errno);

    return ae_err;
  }
  printf("delete file event success for file descriptor = %d, mask = %d\n", fd,
         mask);
  AeFileEvent* fe = fileEvents[fd];
  if (fe->GetMask() == mask) {
    fileEvents[fd] = nullptr;
    delete fe;
    fe = nullptr;
  } else {
    int m = fe->GetMask();
    m &= ~mask;
    fe->SetMask(m);
  }
  return ae_ok;
}

void AeEventLoop::AeCreateTimeEvent(AeTimeEvent* te) {
  if (!timeEventHead) {
    timeEventHead = te;
    return;
  }
  te->SetNext(te);
  timeEventHead = te;
}

void AeEventLoop::AeProcessEvents() {
  if (max_fd == -1) {
    return;
  }
  struct timespec tspec;
  tspec.tv_sec = 1;
  tspec.tv_nsec = 0;
  const std::unordered_map<int, int>& fdToMask = aeApiState->AeApiPoll(&tspec);
  for (const auto& it : fdToMask) {
    int fd = it.first, mask = it.second;
    AeFileEvent* fe = fileEvents[fd];
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
  ProcessTimeEvents();
}

void AeEventLoop::ProcessTimeEvents() const {
  AeTimeEvent* te = timeEventHead;
  while (te) {
    long long id = te->Id();
    if (id == AeFlags::aeDeleteEventId) {
      AeTimeEvent *prev = te->Prev(), *next = te->Next();
      if (prev != nullptr) {
        prev->SetNext(next);
      } else {
        timeEventHead = next;
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
  delete aeApiState;
  aeApiState = nullptr;
  for (int i = 0; i < EventsSize; ++i) {
    delete fileEvents[i];
    fileEvents[i] = nullptr;
  }
}
}  // namespace ae
}  // namespace redis_simple
