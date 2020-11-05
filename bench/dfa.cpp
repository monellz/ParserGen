#include <benchmark/benchmark.h>

#include "dfa.hpp"

static void gen_dfa(benchmark::State& state,  std::string pattern) {
  for (auto _ : state) {
    auto re = parsergen::dfa::DfaEngine::produce(pattern);
  }
}

static void re2dfa(benchmark::State& state, std::string pattern) {
  for (auto _ : state) {
    auto [re, leaf_count] = parsergen::re::ReEngine::produce(pattern);
    auto start = std::chrono::high_resolution_clock::now();

    parsergen::dfa::DfaEngine engine(std::move(re), leaf_count);
    auto dfa = engine.produce();
    dfa.minimize();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    state.SetIterationTime(elapsed_seconds.count());
  }
}

BENCHMARK_CAPTURE(gen_dfa, integer, std::string(R"([1-9][0-9]*)"));
BENCHMARK_CAPTURE(re2dfa, integer, R"([1-9][0-9]*)")->UseManualTime();

BENCHMARK_CAPTURE(gen_dfa, hex, std::string(R"(0x[0-9a-fA-F]+)"));
BENCHMARK_CAPTURE(re2dfa, hex, R"(0x[0-9a-fA-F]+)")->UseManualTime();
BENCHMARK_MAIN();