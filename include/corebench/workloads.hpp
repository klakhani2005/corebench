// Workload kernels used by both the CLI runner and the Google Benchmark suite.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace corebench {

using Values = std::vector<std::uint64_t>;

// Deterministic pseudo-random input so runs are repeatable.
Values make_input(std::size_t n, std::uint64_t seed);

// --- Compute-bound -------------------------------------------------------
// Sorts a copy of the input. Returns the median as a checksum.
std::uint64_t sort_copy(const Values& v);

// Applies a mixing hash to every element (lots of ALU work per byte loaded).
std::uint64_t hash_mix(const Values& v, unsigned threads);

// --- Memory-bound --------------------------------------------------------
// Streams through the array once and sums it, split across threads.
std::uint64_t parallel_sum(const Values& v, unsigned threads);

// --- Cache-bound ---------------------------------------------------------
// Visits elements in a random order given by `order` (defeats the prefetcher),
// versus sequential_gather which visits them in order.
std::uint64_t random_gather(const Values& v, const std::vector<std::uint32_t>& order);
std::uint64_t sequential_gather(const Values& v);
std::vector<std::uint32_t> make_random_order(std::size_t n, std::uint64_t seed);

// --- Synchronization overhead -------------------------------------------
// Every thread increments ONE shared atomic counter (heavy contention).
std::uint64_t shared_atomic_count(std::size_t total_ops, unsigned threads);
// Each thread counts in its own cache-line-padded slot, combined at the end.
std::uint64_t padded_local_count(std::size_t total_ops, unsigned threads);

}  // namespace corebench
