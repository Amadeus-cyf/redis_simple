#include <benchmark/benchmark.h>

#include "benchmarks/buffer.h"
#include "src/memory/reply_buffer.h"

namespace redis_simple {
in_memory::ReplyBuffer replyBuffer;

static void init() {
  for (int i = 0; i < bufferSize; ++i) {
    buffer[i] = rand() % 255;
  }
}

static void ReplyBufferAdd(benchmark::State& state) {
  init();
  for (auto _ : state) {
    replyBuffer.addReplyToBufferOrList(buffer, bufferSize);
  }
}

static void ReplyBufferProcess(benchmark::State& state) {
  for (auto _ : state) {
    replyBuffer.writeProcessed(rand());
  }
}

BENCHMARK(ReplyBufferAdd);
BENCHMARK(ReplyBufferProcess);
}  // namespace redis_simple
