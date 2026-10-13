#include "corebench/workloads.hpp"

#include <algorithm>
#include <atomic>
#include <numeric>
#include <random>
#include <thread>

namespace corebench {

namespace {

// Runs fn(begin, end, thread_index) on `threads` workers, each owning a
// contiguous slice of [0, n).
template <class Fn>
void split_work(std::size_t n, unsigned threads, Fn fn) {
    threads = std::max(1u, std::min<unsigned>(threads, static_cast<unsigned>(std::max<std::size_t>(n, 1))));
    const std::size_t chunk = (n + threads - 1) / threads;
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        const std::size_t begin = std::min(n, t * chunk);
        const std::size_t end = std::min(n, begin + chunk);
        workers.emplace_back(fn, begin, end, t);
    }
    for (auto& w : workers) w.join();
}

inline std::uint64_t mix(std::uint64_t x) {
    // SplitMix64 finalizer, repeated to make the loop compute-heavy.
    for (int i = 0; i < 8; ++i) {
        x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27; x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
    }
    return x;
}

// One counter per cache line so threads never write to the same line.
struct alignas(64) PaddedCounter {
    std::uint64_t value = 0;
};

}  // namespace

Values make_input(std::size_t n, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    Values v(n);
    for (auto& x : v) x = rng();
    return v;
}

std::uint64_t sort_copy(const Values& v) {
    Values copy = v;
    std::sort(copy.begin(), copy.end());
    return copy.empty() ? 0 : copy[copy.size() / 2];
}

std::uint64_t hash_mix(const Values& v, unsigned threads) {
    std::vector<PaddedCounter> partial(std::max(1u, threads));
    split_work(v.size(), threads, [&](std::size_t b, std::size_t e, unsigned t) {
        std::uint64_t acc = 0;
        for (std::size_t i = b; i < e; ++i) acc ^= mix(v[i]);
        partial[t].value = acc;
    });
    std::uint64_t out = 0;
    for (const auto& p : partial) out ^= p.value;
    return out;
}

std::uint64_t parallel_sum(const Values& v, unsigned threads) {
    std::vector<PaddedCounter> partial(std::max(1u, threads));
    split_work(v.size(), threads, [&](std::size_t b, std::size_t e, unsigned t) {
        partial[t].value = std::accumulate(v.begin() + b, v.begin() + e, std::uint64_t{0});
    });
    std::uint64_t out = 0;
    for (const auto& p : partial) out += p.value;
    return out;
}

std::vector<std::uint32_t> make_random_order(std::size_t n, std::uint64_t seed) {
    std::vector<std::uint32_t> order(n);
    std::iota(order.begin(), order.end(), 0u);
    std::shuffle(order.begin(), order.end(), std::mt19937_64(seed));
    return order;
}

std::uint64_t random_gather(const Values& v, const std::vector<std::uint32_t>& order) {
    std::uint64_t sum = 0;
    for (auto idx : order) sum += v[idx];
    return sum;
}

std::uint64_t sequential_gather(const Values& v) {
    std::uint64_t sum = 0;
    for (auto x : v) sum += x;
    return sum;
}

std::uint64_t shared_atomic_count(std::size_t total_ops, unsigned threads) {
    std::atomic<std::uint64_t> counter{0};
    split_work(total_ops, threads, [&](std::size_t b, std::size_t e, unsigned) {
        for (std::size_t i = b; i < e; ++i) counter.fetch_add(1, std::memory_order_relaxed);
    });
    return counter.load();
}

std::uint64_t padded_local_count(std::size_t total_ops, unsigned threads) {
    std::vector<PaddedCounter> slots(std::max(1u, threads));
    split_work(total_ops, threads, [&](std::size_t b, std::size_t e, unsigned t) {
        // volatile stops the compiler from folding the loop into one add.
        volatile std::uint64_t local = 0;
        for (std::size_t i = b; i < e; ++i) local = local + 1;
        slots[t].value = local;
    });
    std::uint64_t out = 0;
    for (const auto& s : slots) out += s.value;
    return out;
}

}  // namespace corebench
