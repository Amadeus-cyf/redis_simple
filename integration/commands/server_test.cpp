#include "server/server.h"

#include <csignal>
#include <cstdint>
#include <cstdlib>

#include "event_loop/loop.h"

namespace redis_simple {
namespace {
// Signal handlers can only safely update simple signal-safe state.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
volatile std::sig_atomic_t g_stop_requested = 0;

void RequestStop(int /*signal*/) { g_stop_requested = 1; }

int StopServerIfRequested(int64_t /*id*/) {
  if (g_stop_requested == 0) {
    return 1;
  }
  Server::Get()->Stop();
  return event_loop::ToInt(event_loop::EventFlag::kNoMore);
}
}  // namespace

int Run() {
  std::signal(SIGINT, RequestStop);
  std::signal(SIGTERM, RequestStop);
  Server::Get()->Loop()->CreateTimeEvent(
      event_loop::TimeEvent::Create(StopServerIfRequested, nullptr));
  return Server::Get()->Run("localhost", 8080) ? EXIT_SUCCESS : EXIT_FAILURE;
}
}  // namespace redis_simple

int main() {
  try {
    return redis_simple::Run();
  } catch (...) {
    return EXIT_FAILURE;
  }
}
