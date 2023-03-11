#include "ae.h"

#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <ctime>
#include <unordered_map>

#include "server/event_loop/ae_kqueue.h"
#include "server/utils/time_utils.h"

namespace redis_simple {
namespace ae {
AeEventLoop::AeEventLoop()
    : fileEvents(new AeFileEvent*[EventsSize]),
      timeEventHead(nullptr),
      aeApiState(nullptr) {}

AeEventLoop::AeEventLoop(AeKqueue* kq)
    : fileEvents(new AeFileEvent*[EventsSize]), aeApiState(kq) {}

AeEventLoop* AeEventLoop::initEventLoop() {
  AeKqueue* kq = AeKqueue::aeApiCreate(EventsSize);
  return new AeEventLoop(kq);
}

void AeEventLoop::aeMain() {
  while (true) {
    aeProcessEvents();
  }
}

int AeEventLoop::aeWait(int fd, int mask, long timeout) {
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

AeStatus AeEventLoop::aeCreateFileEvent(int fd, AeFileEvent* fe) {
  printf("create events for fd = %d, mask = %d\n", fd, fe->getMask());
  if (fe == nullptr) {
    return ae_err;
  }
  if (fd < 0 || fd >= EventsSize) {
    throw("file descriptor out of range");
  }
  int mask = 0;
  if (fileEvents[fd] == nullptr) {
    printf("add new event\n");
    fileEvents[fd] = fe;
    mask = fe->getMask();
  } else {
    /* if a file event already exists, merge the event */
    AeFileEvent* e = fileEvents[fd];
    mask = e->getMask() | fe->getMask();
    fe->setMask(mask);
    if (e->hasRFileProc() && !fe->hasRFileProc()) {
      fe->setRFileProc(e->getRFileProc());
    }
    if (e->hasWFileProc() && !fe->hasWFileProc()) {
      fe->setWFileProc(e->getWFileProc());
    }
    delete e;
    fileEvents[fd] = fe;
  }
  return aeApiState->aeApiAddEvent(fd, mask) < 0 ? ae_err : ae_ok;
}

AeStatus AeEventLoop::aeDelFileEvent(int fd, int mask) {
  if (aeApiState->aeApiDelEvent(fd, mask) < 0) {
    printf(
        "fail to delete the file event of file descriptor %d with errno: "
        "%d\n",
        fd, errno);

    return ae_err;
  }
  printf("delete file event success for file descriptor: %d\n", fd);

  AeFileEvent* fe = fileEvents[fd];
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

void AeEventLoop::aeCreateTimeEvent(AeTimeEvent* te) {
  if (!timeEventHead) {
    timeEventHead = te;
    return;
  }
  te->setNext(te);
  timeEventHead = te;
}

void AeEventLoop::aeProcessEvents() {
  struct timespec tspec;
  tspec.tv_sec = 1;
  tspec.tv_nsec = 0;
  std::unordered_map<int, int> fdToMask = aeApiState->aeApiPoll(&tspec);
  for (const auto& it : fdToMask) {
    int fd = it.first, mask = it.second;
    AeFileEvent* fe = fileEvents[fd];
    int inverted = fe->getMask() & AeFlags::aeBarrier;
    bool fired = false;
    if (!inverted && (mask & fe->getMask() & AeFlags::aeReadable) &&
        fe->hasRFileProc()) {
      fe->getRFileProc()(this, fd, fe->getClientData(), mask);
      fired = true;
    }
    if ((mask & fe->getMask() & AeFlags::aeWritable) && fe->hasWFileProc() &&
        (!fired || fe->getWFileProc() != fe->getRFileProc())) {
      fe->getWFileProc()(this, fd, fe->getClientData(), mask);
      fired = true;
    }
    if (inverted && (mask & fe->getMask() & AeFlags::aeReadable) &&
        fe->hasRFileProc() &&
        (!fired || fe->getRFileProc() != fe->getWFileProc())) {
      fe->getRFileProc()(this, fd, fe->getClientData(), mask);
    }
  }
  processTimeEvents();
}

void AeEventLoop::processTimeEvents() {
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
  delete[] fileEvents;
  fileEvents = nullptr;
}
}  // namespace ae
}  // namespace redis_simple
