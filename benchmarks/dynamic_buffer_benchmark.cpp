#include <benchmark/benchmark.h>

#include "benchmarks/buffer.h"
#include "memory/dynamic_buffer.h"

namespace redis_simple {
in_memory::DynamicBuffer dynamicBuffer;

static void Init() {
  for (int i = 0; i < bufferSize; ++i) {
    buffer[i] = rand() % 1024 == 0 ? '\n' : rand() % 26 + 'a';
  }
}

static void DynamicBufferWrite(benchmark::State& state) {
  Init();
  for (auto _ : state) {
    dynamicBuffer.WriteToBuffer(buffer, bufferSize);
  }
}

static void DynamicBufferProcessAndTrim(benchmark::State& state) {
  for (auto _ : state) {
    const std::string& s = dynamicBuffer.ProcessInlineBuffer();
  }
  dynamicBuffer.TrimProcessedBuffer();
}

BENCHMARK(DynamicBufferWrite);
BENCHMARK(DynamicBufferProcessAndTrim);
}  // namespace redis_simple
