#include <benchmark/benchmark.h>

#include <random>

#include "memory/dict.h"

namespace redis_simple {
std::unique_ptr<in_memory::Dict<std::string, std::string>> g_dict =
    in_memory::Dict<std::string, std::string>::Init();
std::vector<std::string> g_keys;

static std::mt19937& Rng() {
  static std::mt19937 rng(std::mt19937::default_seed);
  return rng;
}

static size_t RandomIndex(size_t size) {
  std::uniform_int_distribution<size_t> dist(0, size - 1);
  return dist(Rng());
}

static std::string RandString(int len) {
  static constexpr char kAlphaNum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::uniform_int_distribution<size_t> dist(0, sizeof(kAlphaNum) - 2);
  std::string str;
  str.reserve(len);

  for (int i = 0; i < len; ++i) {
    str += kAlphaNum[dist(Rng())];
  }

  return str;
}

static void DictAdd(benchmark::State& state) {
  for (auto _ : state) {
    (void)_;
    const auto key = RandString(10);
    g_keys.push_back(key);
    g_dict->Insert(key, RandString(10));
  }
}

static void DictFind(benchmark::State& state) {
  for (auto _ : state) {
    (void)_;
    if (!g_keys.empty()) {
      benchmark::DoNotOptimize(g_dict->Get(g_keys[RandomIndex(g_keys.size())]));
    }
  }
}

static void DictUpdate(benchmark::State& state) {
  std::bernoulli_distribution use_existing(0.5);
  for (auto _ : state) {
    (void)_;
    if (use_existing(Rng()) && !g_keys.empty()) {
      g_dict->Set(g_keys[RandomIndex(g_keys.size())], RandString(10));
    } else {
      g_dict->Set("non-existing key", "val");
    }
  }
}

static void DictDelete(benchmark::State& state) {
  std::bernoulli_distribution use_existing(0.5);
  for (auto _ : state) {
    (void)_;
    if (use_existing(Rng()) && !g_keys.empty()) {
      g_dict->Delete(g_keys[RandomIndex(g_keys.size())]);
    } else {
      g_dict->Delete("non-existing key");
    }
  }
}

BENCHMARK(DictAdd);
BENCHMARK(DictFind);
BENCHMARK(DictUpdate);
BENCHMARK(DictDelete);
}  // namespace redis_simple

BENCHMARK_MAIN();
