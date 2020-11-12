#include <benchmark/benchmark.h>



#include <bitset>
#include <set>
#include <unordered_set>


#include "common.hpp"

#define SET_NUM 1000
using namespace parsergen;

static void std_bitset_insert(benchmark::State& state) {
  std::bitset<SET_NUM> s;
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.set(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void std_bitset_erase(benchmark::State& state) {
  std::bitset<SET_NUM> s;
  for (int i = 0; i < SET_NUM; ++i) s.set(rand() % SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.reset(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void std_intset_insert(benchmark::State& state) {
  std::set<int> s;
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.insert(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void std_intset_erase(benchmark::State& state) {
  std::set<int> s;
  for (int i = 0; i < SET_NUM; ++i) s.insert(rand() % SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.erase(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void std_inthsset_insert(benchmark::State& state) {
  std::unordered_set<int> s;
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.insert(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void std_inthsset_erase(benchmark::State& state) {
  std::unordered_set<int> s;
  for (int i = 0; i < SET_NUM; ++i) s.insert(rand() % SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.erase(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void intset8_insert(benchmark::State& state) {
  IntSet<uint8_t> s(SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.insert(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}
static void intset8_erase(benchmark::State& state) {
  IntSet<uint8_t> s(SET_NUM);
  for (int i = 0; i < SET_NUM; ++i) s.insert(rand() % SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.erase(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void intset16_insert(benchmark::State& state) {
  IntSet<uint16_t> s(SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.insert(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}
static void intset16_erase(benchmark::State& state) {
  IntSet<uint16_t> s(SET_NUM);
  for (int i = 0; i < SET_NUM; ++i) s.insert(rand() % SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.erase(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void intset32_insert(benchmark::State& state) {
  IntSet<uint32_t> s(SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.insert(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}
static void intset32_erase(benchmark::State& state) {
  IntSet<uint32_t> s(SET_NUM);
  for (int i = 0; i < SET_NUM; ++i) s.insert(rand() % SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.erase(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

static void intset64_insert(benchmark::State& state) {
  IntSet<uint64_t> s(SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.insert(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}
static void intset64_erase(benchmark::State& state) {
  IntSet<uint64_t> s(SET_NUM);
  for (int i = 0; i < SET_NUM; ++i) s.insert(rand() % SET_NUM);
  for (auto _ : state) {
    // This code gets timed
    int k = rand() % SET_NUM;
    auto start = std::chrono::high_resolution_clock::now();
    s.erase(k);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK(intset8_insert)->UseManualTime();
BENCHMARK(intset8_erase)->UseManualTime();
BENCHMARK(intset16_insert)->UseManualTime();
BENCHMARK(intset16_erase)->UseManualTime();
BENCHMARK(intset32_insert)->UseManualTime();
BENCHMARK(intset32_erase)->UseManualTime();
BENCHMARK(intset64_insert)->UseManualTime();
BENCHMARK(intset64_erase)->UseManualTime();
BENCHMARK(std_bitset_insert)->UseManualTime();
BENCHMARK(std_bitset_erase)->UseManualTime();
BENCHMARK(std_intset_insert)->UseManualTime();
BENCHMARK(std_intset_erase)->UseManualTime();
BENCHMARK(std_inthsset_insert)->UseManualTime();
BENCHMARK(std_inthsset_erase)->UseManualTime();
BENCHMARK_MAIN();