#include <benchmark/benchmark.h>

#include "benchmarks/buffer.h"
#include "src/memory/query_buffer.h"

namespace redis_simple {
in_memory::QueryBuffer queryBuffer;

static void init() {
  for (int i = 0; i < bufferSize; ++i) {
    buffer[i] = rand() % 1024 == 0 ? '\n' : rand() % 26 + 'a';
  }
}

static void QueryBufferWrite(benchmark::State& state) {
  init();
  for (auto _ : state) {
    queryBuffer.writeToBuffer(buffer, bufferSize);
  }
}

static void QueryBufferProcessAndTrim(benchmark::State& state) {
  for (auto _ : state) {
    const std::string& s = queryBuffer.processInlineBuffer();
  }
  queryBuffer.trimProcessedBuffer();
}

BENCHMARK(QueryBufferWrite);
BENCHMARK(QueryBufferProcessAndTrim);
}  // namespace redis_simple
