#include "ae.h"

#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <ctime>
#include <unordered_map>

#include "server/event_loop/ae_kqueue.h"
#include "utils/time_utils.h"

namespace redis_simple {
namespace ae {
AeEventLoop::AeEventLoop(AeKqueue* kq)
    : fileEvents(std::vector<BaseAeFileEvent*>(EventsSize)), aeApiState(kq) {}

std::unique_ptr<const AeEventLoop> AeEventLoop::initEventLoop() {
  AeKqueue* kq = AeKqueue::aeApiCreate(EventsSize);
  return std::unique_ptr<const AeEventLoop>(new AeEventLoop(kq));
}

void AeEventLoop::aeMain() const {
  while (true) {
    aeProcessEvents();
  }
}

int AeEventLoop::aeWait(int fd, int mask, long timeout) const {
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

AeStatus AeEventLoop::aeDeleteFileEvent(int fd, int mask) const {
  if (aeApiState->aeApiDelEvent(fd, mask) < 0) {
    printf(
        "fail to delete the file event of file descriptor %d with errno: "
        "%d\n",
        fd, errno);

    return ae_err;
  }
  printf("delete file event success for file descriptor = %d, mask = %d\n", fd,
         mask);
  BaseAeFileEvent* fe = fileEvents[fd];
  if (fe->getMask() == mask) {
    fileEvents[fd] = nullptr;
    delete fe;
    fe = nullptr;
  } else {
    int m = fe->getMask();
    m &= ~mask;
    fe->setMask(m);
  }
  return ae_ok;
}

void AeEventLoop::aeCreateTimeEvent(AeTimeEvent* te) const {
  if (!timeEventHead) {
    timeEventHead = te;
    return;
  }
  te->setNext(te);
  timeEventHead = te;
}

void AeEventLoop::aeProcessEvents() const {
  struct timespec tspec;
  tspec.tv_sec = 1;
  tspec.tv_nsec = 0;
  std::unordered_map<int, int> fdToMask = aeApiState->aeApiPoll(&tspec);
  for (const auto& it : fdToMask) {
    int fd = it.first, mask = it.second;
    BaseAeFileEvent* fe = fileEvents[fd];
    int inverted = fe->getMask() & AeFlags::aeBarrier;
    bool fired = false;
    if (!inverted && (mask & fe->getMask() & AeFlags::aeReadable) &&
        fe->hasRFileProc()) {
      fe->callReadProc(this, fd);
      fired = true;
    }
    if ((mask & fe->getMask() & AeFlags::aeWritable) && fe->hasWFileProc() &&
        (!fired || fe->isRWProcDiff())) {
      fe->callWriteProc(this, fd);
      fired = true;
    }
    if (inverted && (mask & fe->getMask() & AeFlags::aeReadable) &&
        fe->hasRFileProc() && (!fired || fe->isRWProcDiff())) {
      fe->callReadProc(this, fd);
    }
  }
  processTimeEvents();
}

void AeEventLoop::processTimeEvents() const {
  AeTimeEvent* te = timeEventHead;
  while (te) {
    long long id = te->getId();
    if (id == AeFlags::aeDeleteEventId) {
      AeTimeEvent *prev = te->getPrev(), *next = te->getNext();
      if (prev != nullptr) {
        prev->setNext(next);
      } else {
        timeEventHead = next;
      }
      if (next != nullptr) {
        next->setPrev(prev);
      }
      if (te->hasTimeFinalizeProc()) {
        te->getTimeFinalizeProc()(te->getClientData());
      }
      delete te;
      te = next;
    } else {
      int64_t now = utils::getNowInMilliseconds();
      if (te->getWhen() <= now) {
        int ret = te->getTimeProc()(id, te->getClientData());
        if (ret == AeFlags::aeNoMore) {
          te->setId(AeFlags::aeDeleteEventId);
        } else {
          te->setWhen(now + ret * 1000);
        }
      }
      te = te->getNext();
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
