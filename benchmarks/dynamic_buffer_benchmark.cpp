#include <benchmark/benchmark.h>

#include "benchmarks/buffer.h"
#include "memory/dynamic_buffer.h"

namespace redis_simple {
in_memory::DynamicBuffer dynamic_buffer;

static void Init() {
  for (int i = 0; i < kBufferSize; ++i) {
    buffer[i] = rand() % 1024 == 0 ? '\n' : rand() % 26 + 'a';
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
    const std::string& s = dynamic_buffer.ProcessInlineBuffer();
  }
  dynamic_buffer.TrimProcessedBuffer();
}

BENCHMARK(DynamicBufferWrite);
BENCHMARK(DynamicBufferProcessAndTrim);
}  // namespace redis_simple
