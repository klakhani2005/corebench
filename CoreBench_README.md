# CoreBench — C++ Performance Benchmarking Toolkit

Measures compute-, memory-, cache- and synchronization-bound workloads across
input sizes and thread counts, with correctness checks on every run.

| Workload | Category | What it shows |
|---|---|---|
| `std_sort` | compute | baseline single-thread cost |
| `hash_mix` | compute | ALU-heavy loop, should scale with cores |
| `parallel_sum` | memory | streaming read, limited by memory bandwidth, not cores |
| `sequential_access` vs `random_access` | cache | cost of cache misses (prefetcher vs random order) |
| `shared_atomic` vs `padded_local` | sync | contention on one atomic vs per-thread cache-line-padded counters |

## Build
Requires CMake 3.16+, a C++20 compiler, and internet on first build (downloads Google Benchmark).
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure    # correctness tests
```
No internet? Add `-DCOREBENCH_GBENCH=OFF` to skip the Google Benchmark target.

## Run
```bash
# CSV runner: every workload at 1, 2, 4 and 8 threads, 30 repetitions
./build/corebench --size 2000000 --repetitions 30 --threads 1,2,4,8 > results.csv
python3 scripts/analyze.py results.csv --plot   # table + speedup vs 1 thread + scaling.png

# Google Benchmark suite (auto-calibrated iterations)
./build/corebench_gbench
./build/corebench_gbench --benchmark_filter=ParallelSum --benchmark_repetitions=10
```

## Profiling with Linux perf
```bash
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses \
    ./build/corebench --size 2000000 --repetitions 5 --threads 1
perf record -g ./build/corebench_gbench --benchmark_filter=RandomAccess && perf report
```
Compare `sequential_access` vs `random_access` cache-miss counts to explain the gap.

## Notes on results
- Keep machine, input size, build type and background load the same when comparing.
- `cv%` in `analyze.py` is run-to-run variation. High values mean noisy measurements.
- `parallel_sum` stops scaling early because it is memory-bound, not compute-bound.
