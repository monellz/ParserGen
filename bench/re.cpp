#include <benchmark/benchmark.h>

#include "re.hpp"

static void parse_integer(benchmark::State& state) {
  // Perform setup here
  std::string pattern = R"([1-9][0-9]*)";

  for (auto _ : state) {
    // This code gets timed
    auto [re, leaf_count] = parsergen::re::ReEngine::produce(pattern);
  }
}

static void parse_hex(benchmark::State& state) {
  std::string pattern = R"(0x[0-9a-fA-F]+)";
  for (auto _ : state) {
    auto [re, leaf_count] = parsergen::re::ReEngine::produce(pattern);
  }
}

// Register the function as a benchmark
BENCHMARK(parse_integer);
BENCHMARK(parse_hex);
// Run the benchmark
BENCHMARK_MAIN();