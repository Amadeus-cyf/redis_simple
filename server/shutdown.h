#pragma once

namespace redis_simple::shutdown {
bool InstallSignalHandlers();
bool StopRequested();
}  // namespace redis_simple::shutdown
