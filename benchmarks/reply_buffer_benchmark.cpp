#include <benchmark/benchmark.h>

#include "benchmarks/buffer.h"
#include "memory/reply_buffer.h"

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
    replyBuffer.writeProcessed(
        std::min((size_t)rand(), replyBuffer.getReplyBytes()));
  }
}

BENCHMARK(ReplyBufferAdd);
BENCHMARK(ReplyBufferProcess);
}  // namespace redis_simple
