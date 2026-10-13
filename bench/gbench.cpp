// Google Benchmark suite for the same kernels. Build with -DCOREBENCH_GBENCH=ON.
#include "corebench/workloads.hpp"

#include <benchmark/benchmark.h>

using namespace corebench;

static void BM_StdSort(benchmark::State& state) {
    const auto v = make_input(static_cast<std::size_t>(state.range(0)), 42);
    for (auto _ : state) benchmark::DoNotOptimize(sort_copy(v));
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StdSort)->RangeMultiplier(10)->Range(10'000, 1'000'000)->Unit(benchmark::kMillisecond);

static void BM_SequentialAccess(benchmark::State& state) {
    const auto v = make_input(static_cast<std::size_t>(state.range(0)), 42);
    for (auto _ : state) benchmark::DoNotOptimize(sequential_gather(v));
    state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(std::uint64_t));
}
BENCHMARK(BM_SequentialAccess)->RangeMultiplier(10)->Range(10'000, 10'000'000);

static void BM_RandomAccess(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto v = make_input(n, 42);
    const auto order = make_random_order(n, 43);
    for (auto _ : state) benchmark::DoNotOptimize(random_gather(v, order));
    state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(std::uint64_t));
}
BENCHMARK(BM_RandomAccess)->RangeMultiplier(10)->Range(10'000, 10'000'000);

static void BM_ParallelSum(benchmark::State& state) {
    const auto v = make_input(10'000'000, 42);
    const auto threads = static_cast<unsigned>(state.range(0));
    for (auto _ : state) benchmark::DoNotOptimize(parallel_sum(v, threads));
}
BENCHMARK(BM_ParallelSum)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime()->Unit(benchmark::kMillisecond);

static void BM_HashMix(benchmark::State& state) {
    const auto v = make_input(1'000'000, 42);
    const auto threads = static_cast<unsigned>(state.range(0));
    for (auto _ : state) benchmark::DoNotOptimize(hash_mix(v, threads));
}
BENCHMARK(BM_HashMix)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime()->Unit(benchmark::kMillisecond);

static void BM_SharedAtomic(benchmark::State& state) {
    const auto threads = static_cast<unsigned>(state.range(0));
    for (auto _ : state) benchmark::DoNotOptimize(shared_atomic_count(1'000'000, threads));
}
BENCHMARK(BM_SharedAtomic)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime()->Unit(benchmark::kMillisecond);

static void BM_PaddedLocal(benchmark::State& state) {
    const auto threads = static_cast<unsigned>(state.range(0));
    for (auto _ : state) benchmark::DoNotOptimize(padded_local_count(1'000'000, threads));
}
BENCHMARK(BM_PaddedLocal)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->UseRealTime()->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
