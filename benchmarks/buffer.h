#pragma once

#include <cstddef>

namespace redis_simple {
static constexpr size_t kBufferSize = 4096;
static char g_buffer[kBufferSize];
}  // namespace redis_simple
