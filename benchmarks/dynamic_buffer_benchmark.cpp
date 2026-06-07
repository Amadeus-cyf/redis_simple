#include <benchmark/benchmark.h>

#include <random>

#include "benchmarks/buffer.h"
#include "memory/dynamic_buffer.h"

namespace redis_simple {
in_memory::DynamicBuffer dynamic_buffer;

static void Init() {
  static std::mt19937 rng(std::mt19937::default_seed);
  std::uniform_int_distribution<int> letter_dist(0, 25);
  std::uniform_int_distribution<int> newline_dist(0, 1023);
  for (int i = 0; i < kBufferSize; ++i) {
    buffer[i] = newline_dist(rng) == 0 ? '\n' : letter_dist(rng) + 'a';
  }
}

static void DynamicBufferWrite(benchmark::State& state) {
  Init();
  for (auto _ : state) {
    dynamic_buffer.WriteToBuffer(buffer, kBufferSize);
  }
}

static void DynamicBufferProcessAndTrim(benchmark::State& state) {
  for (auto _ : state) {
    const auto s = dynamic_buffer.ProcessInlineBuffer();
    benchmark::DoNotOptimize(s.data());
    benchmark::DoNotOptimize(s.size());
  }
  dynamic_buffer.TrimProcessedBuffer();
}

BENCHMARK(DynamicBufferWrite);
BENCHMARK(DynamicBufferProcessAndTrim);
}  // namespace redis_simple
