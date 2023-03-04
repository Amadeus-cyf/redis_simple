#include <benchmark/benchmark.h>

#include "server/memory/dict.h"

namespace redis_simple {
std::unique_ptr<in_memory::Dict<std::string, std::string>> dict =
    in_memory::Dict<std::string, std::string>::init();
std::vector<std::string> keys;

std::string randString(const int len) {
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
    const std::string& key = randString(10);
    keys.push_back(key);
    dict->add(key, randString(10));
  }
}

static void DictFind(benchmark::State& state) {
  for (auto _ : state) {
    dict->find(keys[rand() % keys.size()]);
  }
}

static void DictUpdate(benchmark::State& state) {
  bool exist = false;
  for (auto _ : state) {
    exist = rand() % 2 == 1;
    if (exist) {
      dict->replace(keys[rand() % keys.size()], randString(10));
    } else {
      dict->replace("non-existing key", "val");
    }
  }
}

static void DictDelete(benchmark::State& state) {
  bool exist = false;
  for (auto _ : state) {
    exist = rand() % 2 == 1;
    if (exist) {
      dict->del(keys[rand() % keys.size()]);
    } else {
      dict->del("non-existing key");
    }
  }
}

BENCHMARK(DictAdd);
BENCHMARK(DictFind);
BENCHMARK(DictUpdate);
BENCHMARK(DictDelete);
}  // namespace redis_simple

BENCHMARK_MAIN();
