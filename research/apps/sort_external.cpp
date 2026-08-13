#include "sortlab/extended_algorithms.hpp"
#include "sortlab/workloads.hpp"

#include "pdqsort.h"
#include <ips4o.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace sortlab;

#ifndef SORTLAB_PDQ_COMMIT
#define SORTLAB_PDQ_COMMIT "unknown"
#endif
#ifndef SORTLAB_IPS4O_COMMIT
#define SORTLAB_IPS4O_COMMIT "unknown"
#endif

namespace {
using ExternalFn = void (*)(std::vector<Value>&);

void run_pdq(std::vector<Value>& values) { pdqsort(values.begin(), values.end()); }
void run_pdq_branchless(std::vector<Value>& values) { pdqsort_branchless(values.begin(), values.end()); }
void run_ips4o(std::vector<Value>& values) { ips4o::sort(values.begin(), values.end()); }
void run_intro(std::vector<Value>& values) { Stats stats; intro_sort<false>(values, stats); }
void run_quick3(std::vector<Value>& values) { Stats stats; quick3_sort<false>(values, stats); }
void run_radix11(std::vector<Value>& values) { Stats stats; radix_lsd_11_sort<false>(values, stats); }
void run_std(std::vector<Value>& values) { std::sort(values.begin(), values.end()); }

struct Entry {
  std::string_view name;
  std::string_view origin;
  std::string_view commit;
  std::string_view license;
  ExternalFn sort;
};

const std::vector<Entry>& entries() {
  static const std::vector<Entry> table = {
      {"pdqsort", "orlp/pdqsort", SORTLAB_PDQ_COMMIT, "zlib", run_pdq},
      {"pdqsort_branchless", "orlp/pdqsort", SORTLAB_PDQ_COMMIT, "zlib", run_pdq_branchless},
      {"ips4o_sequential", "ips4o/ips4o", SORTLAB_IPS4O_COMMIT, "BSD-2-Clause", run_ips4o},
      {"intro", "sortlab-core", "local", "MIT", run_intro},
      {"quick_3way", "sortlab-core", "local", "MIT", run_quick3},
      {"radix_lsd_11", "sortlab-core", "local", "MIT", run_radix11},
      {"std_sort", "c++-standard-library", "implementation", "implementation", run_std},
  };
  return table;
}

std::vector<std::size_t> selected_entries(const std::vector<std::string>& names) {
  std::vector<std::size_t> indexes;
  if (names.empty()) {
    indexes.resize(entries().size());
    std::iota(indexes.begin(), indexes.end(), 0);
    return indexes;
  }
  for (const auto& name : names) {
    const auto it = std::find_if(entries().begin(), entries().end(), [&](const Entry& e) { return e.name == name; });
    if (it == entries().end()) throw std::runtime_error("unknown algorithm: " + name);
    indexes.push_back(static_cast<std::size_t>(std::distance(entries().begin(), it)));
  }
  return indexes;
}

int self_test() {
  std::vector<std::vector<Value>> cases = {{}, {1}, {2, 1}, {1, 1, 1}, {3, -1, 2, 2, 0},
      {std::numeric_limits<Value>::min(), 0, std::numeric_limits<Value>::max()}};
  for (const auto& pattern : all_patterns()) {
    for (const std::size_t n : {0U, 1U, 2U, 17U, 127U})
      cases.push_back(make_data(pattern, n, trial_seed(424242, pattern, n, 3)));
  }
  for (const auto& entry : entries()) {
    for (const auto& input : cases) {
      auto copy = input;
      entry.sort(copy);
      if (!verify(input, copy)) {
        std::cerr << "FAIL " << entry.name << " n=" << input.size() << '\n';
        return 1;
      }
    }
  }
  std::cout << "PASS: " << entries().size() << " external-track algorithms/controls across "
            << all_patterns().size() << " workload families\n";
  return 0;
}

void print_algorithms() {
  std::cout << "algorithm,origin,upstream_commit,license\n";
  for (const auto& e : entries())
    std::cout << e.name << ',' << e.origin << ',' << e.commit << ',' << e.license << '\n';
}

void print_environment() {
  std::cout << "{\"schema_version\":1,\"track\":\"external-v1\",\"pdqsort_commit\":\""
            << SORTLAB_PDQ_COMMIT << "\",\"ips4o_commit\":\"" << SORTLAB_IPS4O_COMMIT
            << "\",\"algorithm_count\":" << entries().size() << "}\n";
}
}

int main(int argc, char** argv) {
  try {
    bool do_test = false, list = false, environment = false;
    std::uint64_t seed = 0x5EEDULL;
    int trials = 11, warmups = 1;
    std::vector<std::size_t> sizes = {32, 256, 2048, 16384, 131072};
    std::vector<std::string> algorithm_names, pattern_names;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--self-test") do_test = true;
      else if (arg == "--list-algorithms") list = true;
      else if (arg == "--environment") environment = true;
      else if (arg == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
      else if (arg == "--trials" && i + 1 < argc) trials = std::stoi(argv[++i]);
      else if (arg == "--warmups" && i + 1 < argc) warmups = std::stoi(argv[++i]);
      else if (arg == "--sizes" && i + 1 < argc) sizes = parse_sizes(argv[++i]);
      else if (arg == "--algorithms" && i + 1 < argc) algorithm_names = split_csv(argv[++i]);
      else if (arg == "--patterns" && i + 1 < argc) pattern_names = split_csv(argv[++i]);
      else if (arg == "--help") {
        std::cout << "sort_external [--self-test] [--list-algorithms] [--environment] [--seed N] "
                     "[--trials N] [--warmups N] [--sizes a,b] [--algorithms a,b] [--patterns a,b]\n";
        return 0;
      } else throw std::runtime_error("unknown or incomplete argument: " + arg);
    }
    if (do_test) return self_test();
    if (list) { print_algorithms(); return 0; }
    if (environment) { print_environment(); return 0; }
    if (trials < 1 || warmups < 0 || sizes.empty()) throw std::runtime_error("invalid experiment dimensions");
    const auto selected = selected_entries(algorithm_names);
    const auto patterns = selected_patterns(pattern_names);
    std::cout << "schema_version,algorithm,origin,upstream_commit,license,pattern,n,trial,experiment_seed,trial_seed,input_hash,execution_order,ns,verified\n";
    for (const auto n : sizes) for (const auto& pattern : patterns) {
      for (int warmup = 0; warmup < warmups; ++warmup) {
        const auto s = trial_seed(seed ^ 0xA0761D6478BD642FULL, pattern, n, static_cast<std::uint64_t>(warmup));
        const auto input = make_data(pattern, n, s);
        for (const auto index : selected) {
          auto copy = input; entries()[index].sort(copy);
          if (!verify(input, copy)) throw std::runtime_error("warmup verification failed");
        }
      }
      for (int trial = 0; trial < trials; ++trial) {
        const auto s = trial_seed(seed, pattern, n, static_cast<std::uint64_t>(trial));
        const auto input = make_data(pattern, n, s);
        const auto fingerprint = input_hash(input);
        auto order = selected;
        deterministic_shuffle(order, splitmix64(s ^ 0xE7037ED1A0B428DBULL));
        std::size_t execution_order = 0;
        for (const auto index : order) {
          const auto& e = entries()[index];
          auto copy = input;
          const auto start = std::chrono::steady_clock::now();
          e.sort(copy);
          const auto stop = std::chrono::steady_clock::now();
          const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
          const bool ok = verify(input, copy);
          if (!ok) throw std::runtime_error("verification failed for " + std::string(e.name));
          std::cout << 1 << ',' << e.name << ',' << e.origin << ',' << e.commit << ',' << e.license << ','
                    << pattern << ',' << n << ',' << trial << ',' << seed << ',' << s << ',' << fingerprint << ','
                    << execution_order++ << ',' << ns << ',' << 1 << '\n';
        }
      }
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 2;
  }
}
