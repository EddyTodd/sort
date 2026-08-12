#include "sortlab/extended_algorithms.hpp"
#include "sortlab/perf_counters.hpp"
#include "sortlab/workloads.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef __linux__
#include <sched.h>
#endif

using namespace sortlab;

namespace {

std::vector<std::string> split(std::string_view text) {
  std::vector<std::string> result;
  std::size_t position = 0;
  while (position <= text.size()) {
    const auto end = text.find(',', position);
    const auto token = text.substr(
        position, end == std::string_view::npos ? text.size() - position : end - position);
    if (token.empty()) throw std::runtime_error("empty item");
    result.emplace_back(token);
    if (end == std::string_view::npos) break;
    position = end + 1;
  }
  return result;
}

std::vector<std::size_t> numbers(std::string_view text) {
  std::vector<std::size_t> result;
  for (const auto& token : split(text)) result.push_back(static_cast<std::size_t>(std::stoull(token)));
  return result;
}

std::vector<std::size_t> select_algorithms(const std::vector<std::string>& names) {
  const auto& table = all_algorithms();
  std::vector<std::size_t> result;
  if (names.empty()) {
    result.resize(table.size());
    std::iota(result.begin(), result.end(), 0);
    return result;
  }
  for (const auto& name : names) {
    const auto it = std::find_if(table.begin(), table.end(),
                                 [&](const Algorithm& algorithm) { return algorithm.name == name; });
    if (it == table.end()) throw std::runtime_error("unknown algorithm: " + name);
    result.push_back(static_cast<std::size_t>(std::distance(table.begin(), it)));
  }
  return result;
}

void pin_cpu(int cpu) {
  if (cpu < 0) return;
#ifdef __linux__
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpu, &set);
  if (sched_setaffinity(0, sizeof(set), &set) != 0) {
    throw std::runtime_error("sched_setaffinity failed");
  }
#else
  throw std::runtime_error("--cpu requires Linux");
#endif
}

int self_test() {
  const auto input = make_data("random", 257, trial_seed(1, "random", 257, 0));
  for (const std::string name : {"insertion", "merge_insertion_24", "dual_pivot",
                                 "radix_lsd_11", "std_sort"}) {
    const auto index = select_algorithms({name}).front();
    auto copy = input;
    Stats stats;
    all_algorithms()[index].timed(copy, stats);
    if (!verify(input, copy)) return 1;
  }
  PerfCounters counters;
  std::cout << "PASS: perf harness; counters_available=" << (counters.available() ? 1 : 0) << '\n';
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    bool test = false;
    int trials = 11;
    int cpu = -1;
    std::uint64_t seed = 0x5EEDULL;
    std::vector<std::size_t> sizes = {1024, 16384, 262144};
    std::vector<std::string> patterns = {"random", "few_unique", "nearly_sorted"};
    std::vector<std::string> names = {"intro", "merge_insertion_24", "dual_pivot",
                                      "radix_lsd_11", "std_sort"};

    for (int i = 1; i < argc; ++i) {
      const std::string argument = argv[i];
      if (argument == "--self-test") test = true;
      else if (argument == "--trials" && i + 1 < argc) trials = std::stoi(argv[++i]);
      else if (argument == "--cpu" && i + 1 < argc) cpu = std::stoi(argv[++i]);
      else if (argument == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
      else if (argument == "--sizes" && i + 1 < argc) sizes = numbers(argv[++i]);
      else if (argument == "--patterns" && i + 1 < argc) patterns = split(argv[++i]);
      else if (argument == "--algorithms" && i + 1 < argc) names = split(argv[++i]);
      else if (argument == "--help") {
        std::cout << "sort_perf [--cpu N] [--trials N] [--sizes csv] [--patterns csv] "
                     "[--algorithms csv]\n";
        return 0;
      } else {
        throw std::runtime_error("bad argument: " + argument);
      }
    }

    if (test) return self_test();
    if (trials < 1) throw std::runtime_error("trials must be positive");
    pin_cpu(cpu);

    const auto selected = select_algorithms(names);
    const auto& table = all_algorithms();
    PerfCounters counters;
    if (!counters.available()) {
      std::cerr << "hardware counters unavailable: " << counters.reason() << '\n';
    }

    std::cout << "schema_version,algorithm,pattern,n,trial,trial_seed,input_hash,"
                 "execution_order,ns,perf_available,cycles,instructions,branches,branch_misses,"
                 "cache_references,cache_misses,verified\n";

    for (const std::size_t n : sizes) {
      for (const auto& pattern : patterns) {
        for (int trial = 0; trial < trials; ++trial) {
          const auto seed_value = trial_seed(seed, pattern, n, static_cast<std::uint64_t>(trial));
          const auto input = make_data(pattern, n, seed_value);
          const auto fingerprint = input_hash(input);
          auto order = selected;
          deterministic_shuffle(order, splitmix64(seed_value ^ 0x94D049BB133111EBULL));
          std::size_t execution_order = 0;

          for (const std::size_t index : order) {
            auto copy = input;
            Stats stats;
            counters.start();
            const auto start = std::chrono::steady_clock::now();
            table[index].timed(copy, stats);
            const auto stop = std::chrono::steady_clock::now();
            const auto perf = counters.stop();
            const bool verified = verify(input, copy);
            std::cout << 1 << ',' << table[index].name << ',' << pattern << ',' << n << ','
                      << trial << ',' << seed_value << ',' << fingerprint << ','
                      << execution_order++ << ','
                      << std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count()
                      << ',' << (perf.available ? 1 : 0) << ',' << perf.cycles << ','
                      << perf.instructions << ',' << perf.branches << ',' << perf.branch_misses
                      << ',' << perf.cache_references << ',' << perf.cache_misses << ','
                      << (verified ? 1 : 0) << '\n';
            if (!verified) return 1;
          }
        }
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 2;
  }
}
