#include <benchmark/benchmark.h>

#include <random>

#include "memory/dict.h"

namespace redis_simple {
std::unique_ptr<in_memory::Dict<std::string, std::string>> dict =
    in_memory::Dict<std::string, std::string>::Init();
std::vector<std::string> keys;

static std::mt19937& Rng() {
  static std::mt19937 rng(std::mt19937::default_seed);
  return rng;
}

static size_t RandomIndex(size_t size) {
  std::uniform_int_distribution<size_t> dist(0, size - 1);
  return dist(Rng());
}

static std::string RandString(const int len) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::uniform_int_distribution<size_t> dist(0, sizeof(alphanum) - 2);
  std::string str;
  str.reserve(len);

  for (int i = 0; i < len; ++i) {
    str += alphanum[dist(Rng())];
  }

  return str;
}

static void DictAdd(benchmark::State& state) {
  for (auto _ : state) {
    const auto key = RandString(10);
    keys.push_back(key);
    dict->Insert(key, RandString(10));
  }
}

static void DictFind(benchmark::State& state) {
  for (auto _ : state) {
    if (!keys.empty()) {
      benchmark::DoNotOptimize(dict->Get(keys[RandomIndex(keys.size())]));
    }
  }
}

static void DictUpdate(benchmark::State& state) {
  std::bernoulli_distribution use_existing(0.5);
  for (auto _ : state) {
    if (use_existing(Rng()) && !keys.empty()) {
      dict->Set(keys[RandomIndex(keys.size())], RandString(10));
    } else {
      dict->Set("non-existing key", "val");
    }
  }
}

static void DictDelete(benchmark::State& state) {
  std::bernoulli_distribution use_existing(0.5);
  for (auto _ : state) {
    if (use_existing(Rng()) && !keys.empty()) {
      dict->Delete(keys[RandomIndex(keys.size())]);
    } else {
      dict->Delete("non-existing key");
    }
  }
}

BENCHMARK(DictAdd);
BENCHMARK(DictFind);
BENCHMARK(DictUpdate);
BENCHMARK(DictDelete);
}  // namespace redis_simple

BENCHMARK_MAIN();
