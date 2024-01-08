#include <stdlib.h>
#include <unistd.h>

#include <optional>
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
  const tcp::TCPAddrInfo remote("localhost", 8080);
  const std::optional<const tcp::TCPAddrInfo>& local =
      std::make_optional<const tcp::TCPAddrInfo>("localhost", 8081);
  int fd = tcp::TCP_Connect(remote, local, true);
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
