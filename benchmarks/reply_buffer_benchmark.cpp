#include <benchmark/benchmark.h>

#include "benchmarks/buffer.h"
#include "memory/reply_buffer.h"

namespace redis_simple {
in_memory::ReplyBuffer reply_buffer;

static void Init() {
  for (int i = 0; i < kBufferSize; ++i) {
    buffer[i] = rand() % 255;
  }
}

static void ReplyBufferAdd(benchmark::State& state) {
  Init();
  for (auto _ : state) {
    reply_buffer.AddReplyToBufferOrList(buffer, kBufferSize);
  }
}

static void ReplyBufferProcess(benchmark::State& state) {
  for (auto _ : state) {
    reply_buffer.ClearProcessed(
        std::min((size_t)rand(), reply_buffer.ReplyBytes()));
  }
}

BENCHMARK(ReplyBufferAdd);
BENCHMARK(ReplyBufferProcess);
}  // namespace redis_simple
