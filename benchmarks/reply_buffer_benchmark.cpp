#include <benchmark/benchmark.h>

#include "benchmarks/buffer.h"
#include "memory/reply_buffer.h"

namespace redis_simple {
in_memory::ReplyBuffer replyBuffer;

static void Init() {
  for (int i = 0; i < bufferSize; ++i) {
    buffer[i] = rand() % 255;
  }
}

static void ReplyBufferAdd(benchmark::State& state) {
  Init();
  for (auto _ : state) {
    replyBuffer.AddReplyToBufferOrList(buffer, bufferSize);
  }
}

static void ReplyBufferProcess(benchmark::State& state) {
  for (auto _ : state) {
    replyBuffer.WriteProcessed(
        std::min((size_t)rand(), replyBuffer.ReplyBytes()));
  }
}

BENCHMARK(ReplyBufferAdd);
BENCHMARK(ReplyBufferProcess);
}  // namespace redis_simple
