#include <benchmark/benchmark.h>

#include <random>

#include "benchmarks/buffer.h"
#include "memory/dynamic_buffer.h"

namespace redis_simple {
in_memory::DynamicBuffer g_dynamic_buffer;

static void Init() {
  static std::mt19937 rng(std::mt19937::default_seed);
  std::uniform_int_distribution<int> letter_dist(0, 25);
  std::uniform_int_distribution<int> newline_dist(0, 1023);
  for (char& i : g_buffer) {
    i = newline_dist(rng) == 0 ? '\n'
                               : static_cast<char>(letter_dist(rng) + 'a');
  }
}

static void DynamicBufferWrite(benchmark::State& state) {
  Init();
  for (auto _ : state) {
    (void)_;
    g_dynamic_buffer.WriteToBuffer(g_buffer, kBufferSize);
  }
}

static void DynamicBufferProcessAndTrim(benchmark::State& state) {
  for (auto _ : state) {
    (void)_;
    const auto s = g_dynamic_buffer.ProcessInlineBuffer();
    benchmark::DoNotOptimize(s.data());
    benchmark::DoNotOptimize(s.size());
  }
  g_dynamic_buffer.TrimProcessedBuffer();
}

BENCHMARK(DynamicBufferWrite);
BENCHMARK(DynamicBufferProcessAndTrim);
}  // namespace redis_simple
