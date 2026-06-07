#include <benchmark/benchmark.h>

#include <random>

#include "benchmarks/buffer.h"
#include "memory/reply_buffer.h"

namespace redis_simple {
in_memory::ReplyBuffer reply_buffer;

static std::mt19937& Rng() {
  static std::mt19937 rng(std::mt19937::default_seed);
  return rng;
}

static void Init() {
  std::uniform_int_distribution<int> byte_dist(0, 254);
  for (int i = 0; i < kBufferSize; ++i) {
    buffer[i] = byte_dist(Rng());
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
    std::uniform_int_distribution<size_t> bytes_dist(0,
                                                     reply_buffer.ReplyBytes());
    reply_buffer.ClearProcessed(
        std::min(bytes_dist(Rng()), reply_buffer.ReplyBytes()));
  }
}

BENCHMARK(ReplyBufferAdd);
BENCHMARK(ReplyBufferProcess);
}  // namespace redis_simple
