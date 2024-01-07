#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "event_loop/ae.h"
#include "event_loop/ae_file_event_impl.h"
#include "tcp/tcp.h"

namespace redis_simple {
static int i = 0;
ae::AeEventStatus WriteProc(ae::AeEventLoop* el, int fd, int* client_data,
                            int mask) {
  printf("write to %d\n", fd);
  const std::string& s = "hello world\n";
  ssize_t written = write(fd, s.c_str(), s.size());
  printf("write %zu\n", written);
  ++i;
  if (i == 10) {
    el->AeDeleteFileEvent(fd, ae::AeFlags::aeWritable);
  }
  return ae::AeEventStatus::aeEventOK;
}

void Run() {
  std::shared_ptr<ae::AeEventLoop> el(ae::AeEventLoop::InitEventLoop());
  int fd = tcp::TCP_Connect("localhost", 8080, true, "localhost", 8081);
  printf("conn result %d\n", fd);
  if (fd == -1) {
    printf("connection failed\n");
    return;
  }

  int data = 10000;
  ae::AeFileEvent* fe = ae::AeFileEventImpl<int>::Create(
      nullptr, WriteProc, &data, ae::AeFlags::aeWritable);
  el->AeCreateFileEvent(fd, fe);
  el->AeMain();
}
}  // namespace redis_simple

int main() { redis_simple::Run(); }
