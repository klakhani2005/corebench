// CoreBench CLI: runs every workload across a list of thread counts and
// prints one CSV row per (workload, threads, repetition).
#include "corebench/workloads.hpp"

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct Config {
    std::size_t size = 1'000'000;
    int repetitions = 10;
    std::vector<unsigned> threads = {1, 2, 4, 8};
    std::uint64_t seed = 42;
};

std::uint64_t parse_number(const std::string& text, const std::string& flag) {
    try {
        std::size_t used = 0;
        const auto n = std::stoull(text, &used);
        if (used != text.size()) throw std::invalid_argument("trailing characters");
        return n;
    } catch (...) {
        throw std::invalid_argument("Invalid value for " + flag + ": " + text);
    }
}

std::vector<unsigned> parse_thread_list(const std::string& text) {
    std::vector<unsigned> out;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ',')) {
        const auto n = parse_number(item, "--threads");
        if (n < 1 || n > 256) throw std::invalid_argument("--threads values must be 1..256");
        out.push_back(static_cast<unsigned>(n));
    }
    if (out.empty()) throw std::invalid_argument("--threads needs at least one value");
    return out;
}

void print_help() {
    std::cout << "Usage: corebench [--size N] [--repetitions N] [--threads 1,2,4,8] [--seed N]\n"
                 "Prints CSV: workload,category,input_size,threads,repetition,elapsed_ms,checksum\n";
}

Config parse_args(int argc, char** argv) {
    Config c;
    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];
        if (flag == "--help" || flag == "-h") { print_help(); std::exit(0); }
        if (i + 1 >= argc) throw std::invalid_argument("Missing value for " + flag);
        const std::string value = argv[++i];
        if (flag == "--size") c.size = parse_number(value, flag);
        else if (flag == "--repetitions") c.repetitions = static_cast<int>(parse_number(value, flag));
        else if (flag == "--threads") c.threads = parse_thread_list(value);
        else if (flag == "--seed") c.seed = parse_number(value, flag);
        else throw std::invalid_argument("Unknown argument: " + flag);
    }
    if (c.size < 1 || c.size > 100'000'000) throw std::invalid_argument("--size must be 1..100000000");
    if (c.repetitions < 1 || c.repetitions > 10'000) throw std::invalid_argument("--repetitions must be 1..10000");
    return c;
}

double time_ms(const std::function<void()>& fn) {
    const auto start = Clock::now();
    fn();
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void print_row(const char* name, const char* category, std::size_t n, unsigned threads,
               int rep, double ms, std::uint64_t checksum) {
    std::cout << name << ',' << category << ',' << n << ',' << threads << ',' << rep << ','
              << std::fixed << std::setprecision(4) << ms << ',' << checksum << '\n';
}

void check(bool ok, const char* what) {
    if (!ok) throw std::runtime_error(std::string("validation failed: ") + what);
}

}  // namespace

int main(int argc, char** argv) {
    using namespace corebench;
    try {
        const Config c = parse_args(argc, argv);
        const Values input = make_input(c.size, c.seed);
        const auto order = make_random_order(c.size, c.seed + 1);
        const std::uint64_t expected_sum = sequential_gather(input);
        const std::uint64_t expected_hash = hash_mix(input, 1);

        std::cout << "workload,category,input_size,threads,repetition,elapsed_ms,checksum\n";
        for (int rep = 1; rep <= c.repetitions; ++rep) {
            // Single-threaded workloads: run once per repetition.
            std::uint64_t out = 0;
            double ms = time_ms([&] { out = sort_copy(input); });
            print_row("std_sort", "compute", c.size, 1, rep, ms, out);

            ms = time_ms([&] { out = sequential_gather(input); });
            check(out == expected_sum, "sequential_gather");
            print_row("sequential_access", "cache", c.size, 1, rep, ms, out);

            ms = time_ms([&] { out = random_gather(input, order); });
            check(out == expected_sum, "random_gather");
            print_row("random_access", "cache", c.size, 1, rep, ms, out);

            // Multithreaded workloads: sweep every requested thread count.
            for (unsigned t : c.threads) {
                ms = time_ms([&] { out = parallel_sum(input, t); });
                check(out == expected_sum, "parallel_sum");
                print_row("parallel_sum", "memory", c.size, t, rep, ms, out);

                ms = time_ms([&] { out = hash_mix(input, t); });
                check(out == expected_hash, "hash_mix");
                print_row("hash_mix", "compute", c.size, t, rep, ms, out);

                ms = time_ms([&] { out = shared_atomic_count(c.size, t); });
                check(out == c.size, "shared_atomic_count");
                print_row("shared_atomic", "sync", c.size, t, rep, ms, out);

                ms = time_ms([&] { out = padded_local_count(c.size, t); });
                check(out == c.size, "padded_local_count");
                print_row("padded_local", "sync", c.size, t, rep, ms, out);
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "corebench error: " << e.what() << '\n';
        return 2;
    }
}
