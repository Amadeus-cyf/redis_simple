#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "event_loop/ae.h"
#include "event_loop/ae_file_event_impl.h"
#include "tcp/tcp.h"

namespace redis_simple {
static int i = 0;
ae::AeEventStatus writeProc(ae::AeEventLoop* el, int fd, int* client_data,
                            int mask) {
  printf("write to %d\n", fd);
  const std::string& s = "hello world\n";
  ssize_t written = write(fd, s.c_str(), s.size());
  printf("write %zu\n", written);
  ++i;
  if (i == 10) {
    el->aeDeleteFileEvent(fd, ae::AeFlags::aeWritable);
  }
  return ae::AeEventStatus::aeEventOK;
}

void run() {
  std::unique_ptr<ae::AeEventLoop> el = ae::AeEventLoop::initEventLoop();
  int fd = tcp::tcpConnect("localhost", 8081, true, "localhost", 8080);
  printf("conn result %d\n", fd);
  if (fd == -1) {
    printf("connection failed\n");
    return;
  }

  int data = 10000;
  ae::AeFileEvent* fe = ae::AeFileEventImpl<int>::create(
      nullptr, writeProc, &data, ae::AeFlags::aeWritable);
  el->aeCreateFileEvent(fd, fe);
  el->aeMain();
}
}  // namespace redis_simple

int main() { redis_simple::run(); }
