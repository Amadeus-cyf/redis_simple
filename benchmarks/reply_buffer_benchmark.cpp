#include <benchmark/benchmark.h>

#include <random>

#include "benchmarks/buffer.h"
#include "memory/reply_buffer.h"

namespace redis_simple {
in_memory::ReplyBuffer g_reply_buffer;

static std::mt19937& Rng() {
  static std::mt19937 rng(std::mt19937::default_seed);
  return rng;
}

static void FillBenchmarkBuffer() {
  std::uniform_int_distribution<int> byte_dist(0, 254);
  for (char& i : g_buffer) {
    i = static_cast<char>(byte_dist(Rng()));
  }
}

static void ReplyBufferAdd(benchmark::State& state) {
  FillBenchmarkBuffer();
  for (auto _ : state) {
    (void)_;
    g_reply_buffer.Append(g_buffer, kBufferSize);
  }
}

static void ReplyBufferProcess(benchmark::State& state) {
  for (auto _ : state) {
    (void)_;
    std::uniform_int_distribution<size_t> bytes_dist(
        0, g_reply_buffer.ReplyBytes());
    g_reply_buffer.Consume(
        std::min(bytes_dist(Rng()), g_reply_buffer.ReplyBytes()));
  }
}

BENCHMARK(ReplyBufferAdd);
BENCHMARK(ReplyBufferProcess);
}  // namespace redis_simple
