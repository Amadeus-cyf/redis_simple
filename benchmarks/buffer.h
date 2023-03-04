#pragma once
#include <unistd.h>

namespace redis_simple {
static const constexpr size_t bufferSize = 4096;
static char buffer[bufferSize];
}  // namespace redis_simple
