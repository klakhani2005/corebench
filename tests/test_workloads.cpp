// Minimal correctness tests (no framework needed): run with `ctest`.
#include "corebench/workloads.hpp"

#include <cstdlib>
#include <iostream>

using namespace corebench;

static int failures = 0;
#define EXPECT(cond) do { if (!(cond)) { std::cerr << "FAIL: " #cond " (line " << __LINE__ << ")\n"; ++failures; } } while (0)

int main() {
    const Values v = make_input(10'000, 7);
    const auto expected = sequential_gather(v);

    for (unsigned t : {1u, 2u, 3u, 4u, 8u, 16u}) {
        EXPECT(parallel_sum(v, t) == expected);
        EXPECT(hash_mix(v, t) == hash_mix(v, 1));
        EXPECT(shared_atomic_count(10'000, t) == 10'000);
        EXPECT(padded_local_count(10'000, t) == 10'000);
    }
    EXPECT(random_gather(v, make_random_order(v.size(), 3)) == expected);
    EXPECT(make_input(100, 1) == make_input(100, 1));  // deterministic

    // Edge cases: tiny inputs and more threads than elements.
    const Values one = make_input(1, 1);
    EXPECT(parallel_sum(one, 8) == one[0]);
    EXPECT(sort_copy(one) == one[0]);

    if (failures == 0) std::cout << "All CoreBench tests passed\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
