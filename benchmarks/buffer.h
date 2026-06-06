#pragma once
#include <unistd.h>

namespace redis_simple {
static const constexpr size_t kBufferSize = 4096;
static char buffer[kBufferSize];
}  // namespace redis_simple
