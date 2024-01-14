#include <benchmark/benchmark.h>

#include "memory/dict.h"

namespace redis_simple {
std::unique_ptr<in_memory::Dict<std::string, std::string>> dict =
    in_memory::Dict<std::string, std::string>::Init();
std::vector<std::string> keys;

std::string RandString(const int len) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string str;
  str.reserve(len);

  for (int i = 0; i < len; ++i) {
    str += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return str;
}

static void DictAdd(benchmark::State& state) {
  for (auto _ : state) {
    const std::string& key = RandString(10);
    keys.push_back(key);
    dict->Insert(key, RandString(10));
  }
}

static void DictFind(benchmark::State& state) {
  for (auto _ : state) {
    dict->Get(keys[rand() % keys.size()]);
  }
}

static void DictUpdate(benchmark::State& state) {
  bool exist = false;
  for (auto _ : state) {
    exist = rand() % 2 == 1;
    if (exist) {
      dict->Set(keys[rand() % keys.size()], RandString(10));
    } else {
      dict->Set("non-existing key", "val");
    }
  }
}

static void DictDelete(benchmark::State& state) {
  bool exist = false;
  for (auto _ : state) {
    exist = rand() % 2 == 1;
    if (exist) {
      dict->Delete(keys[rand() % keys.size()]);
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
