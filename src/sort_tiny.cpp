#include "sortlab/tiny_kernels.hpp"
#include "sortlab/workloads.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

using namespace sortlab;

namespace {

struct Kernel {
  std::string name;
  bool network;
};

const std::vector<Kernel>& kernels() {
  static const std::vector<Kernel> table = {
      {"insertion", false},
      {"binary_insertion", false},
      {"bitonic_network", true},
      {"std_sort", false},
  };
  return table;
}

void run_kernel(const std::string& name, std::vector<Value>& values, Stats& stats,
                bool count) {
  if (name == "insertion") {
    if (count) {
      apply_tiny_kernel<true>(values, 0, values.size(), TinyKernel::insertion, stats);
    } else {
      apply_tiny_kernel<false>(values, 0, values.size(), TinyKernel::insertion, stats);
    }
    return;
  }
  if (name == "binary_insertion") {
    if (count) {
      apply_tiny_kernel<true>(values, 0, values.size(),
                              TinyKernel::binary_insertion, stats);
    } else {
      apply_tiny_kernel<false>(values, 0, values.size(),
                               TinyKernel::binary_insertion, stats);
    }
    return;
  }
  if (name == "bitonic_network") {
    if (count) {
      apply_tiny_kernel<true>(values, 0, values.size(),
                              TinyKernel::bitonic_network, stats);
    } else {
      apply_tiny_kernel<false>(values, 0, values.size(),
                               TinyKernel::bitonic_network, stats);
    }
    return;
  }
  if (name == "std_sort") {
    if (count) {
      std::sort(values.begin(), values.end(), [&](Value left, Value right) {
        ++stats.comparisons;
        return left < right;
      });
    } else {
      std::sort(values.begin(), values.end());
    }
    return;
  }
  throw std::runtime_error("unknown kernel: " + name);
}

std::vector<std::size_t> selected_kernels(const std::vector<std::string>& names) {
  std::vector<std::size_t> indexes;
  if (names.empty()) {
    indexes.resize(kernels().size());
    std::iota(indexes.begin(), indexes.end(), 0);
    return indexes;
  }
  for (const auto& name : names) {
    const auto it = std::find_if(kernels().begin(), kernels().end(),
                                 [&](const Kernel& kernel) {
                                   return kernel.name == name;
                                 });
    if (it == kernels().end()) {
      throw std::runtime_error("unknown kernel: " + name);
    }
    indexes.push_back(static_cast<std::size_t>(std::distance(kernels().begin(), it)));
  }
  return indexes;
}

int self_test() {
  std::size_t cases = 0;
  for (const auto& pattern : all_patterns()) {
    for (std::size_t n = 0; n <= 32; ++n) {
      const auto input = make_data(pattern, n, trial_seed(0x7711, pattern, n, 3));
      for (const auto& kernel : kernels()) {
        auto timed = input;
        Stats unused;
        run_kernel(kernel.name, timed, unused, false);
        if (!verify(input, timed)) {
          std::cerr << "FAIL " << kernel.name << ' ' << pattern << " n=" << n
                    << '\n';
          return 1;
        }

        auto instrumented = input;
        Stats stats;
        run_kernel(kernel.name, instrumented, stats, true);
        if (instrumented != timed) {
          std::cerr << "FAIL instrumented parity " << kernel.name << '\n';
          return 1;
        }
      }
      ++cases;
    }
  }

  if (bitonic_gate_count(2) != 1 || bitonic_gate_count(4) != 6 ||
      bitonic_gate_count(8) != 24 || bitonic_gate_count(16) != 80 ||
      bitonic_gate_count(32) != 240) {
    std::cerr << "FAIL bitonic gate-count contract\n";
    return 1;
  }

  std::cout << "PASS: " << kernels().size() << " tiny kernels, "
            << all_patterns().size() << " workloads, " << cases
            << " deterministic cases\n";
  return 0;
}

struct Measurement {
  std::size_t kernel_index{};
  std::size_t execution_order{};
  std::int64_t ns{};
  Stats stats{};
  bool verified{};
};

}  // namespace

int main(int argc, char** argv) {
  try {
    bool do_test = false;
    bool environment = false;
    std::uint64_t seed = 0x5EEDULL;
    int trials = 101;
    int warmups = 2;
    std::vector<std::size_t> sizes = {2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 24, 28, 32};
    std::vector<std::string> kernel_names;
    std::vector<std::string> pattern_names;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--self-test") {
        do_test = true;
      } else if (arg == "--environment") {
        environment = true;
      } else if (arg == "--seed" && i + 1 < argc) {
        seed = std::stoull(argv[++i]);
      } else if (arg == "--trials" && i + 1 < argc) {
        trials = std::stoi(argv[++i]);
      } else if (arg == "--warmups" && i + 1 < argc) {
        warmups = std::stoi(argv[++i]);
      } else if (arg == "--sizes" && i + 1 < argc) {
        sizes = parse_sizes(argv[++i]);
      } else if (arg == "--kernels" && i + 1 < argc) {
        kernel_names = split_csv(argv[++i]);
      } else if (arg == "--patterns" && i + 1 < argc) {
        pattern_names = split_csv(argv[++i]);
      } else if (arg == "--help") {
        std::cout << "sort_tiny [--self-test] [--environment] [--seed N] "
                     "[--trials N] [--warmups N] [--sizes a,b] "
                     "[--kernels a,b] [--patterns a,b]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown or incomplete argument: " + arg);
      }
    }

    if (do_test) return self_test();
    if (environment) {
      std::cout << "{\"schema_version\":1,\"max_network_n\":32,"
                   "\"network\":\"bitonic-padded-power-of-two\","
                   "\"network_data_oblivious_topology\":true}\n";
      return 0;
    }
    if (trials < 1 || warmups < 0) {
      throw std::runtime_error("invalid trial/warmup count");
    }
    for (const auto n : sizes) {
      if (n > 32) throw std::runtime_error("sort_tiny supports n <= 32");
    }

    const auto selected = selected_kernels(kernel_names);
    const auto patterns = selected_patterns(pattern_names);

    std::cout << "schema_version,kernel,pattern,n,trial,experiment_seed,trial_seed,"
                 "input_hash,execution_order,ns,comparisons,swaps,writes,padded_n,"
                 "network_gates,verified\n";

    for (const auto n : sizes) {
      for (const auto& pattern : patterns) {
        for (int warmup = 0; warmup < warmups; ++warmup) {
          const auto warmup_seed =
              trial_seed(seed ^ 0xD1B54A32D192ED03ULL, pattern, n,
                         static_cast<std::uint64_t>(warmup));
          const auto input = make_data(pattern, n, warmup_seed);
          for (const auto index : selected) {
            auto copy = input;
            Stats unused;
            run_kernel(kernels()[index].name, copy, unused, false);
            if (!verify(input, copy)) {
              throw std::runtime_error("warmup verification failed");
            }
          }
        }

        for (int trial = 0; trial < trials; ++trial) {
          const auto current_seed =
              trial_seed(seed, pattern, n, static_cast<std::uint64_t>(trial));
          const auto input = make_data(pattern, n, current_seed);
          const auto fingerprint = input_hash(input);
          auto order = selected;
          deterministic_shuffle(order,
                                splitmix64(current_seed ^ 0xA0761D6478BD642FULL));

          std::vector<Measurement> measurements;
          measurements.reserve(order.size());
          std::size_t execution_order = 0;
          for (const auto index : order) {
            auto copy = input;
            Stats unused;
            const auto start = std::chrono::steady_clock::now();
            run_kernel(kernels()[index].name, copy, unused, false);
            const auto stop = std::chrono::steady_clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
                    .count();
            measurements.push_back(
                {index, execution_order++, elapsed, {}, verify(input, copy)});
            if (!measurements.back().verified) {
              throw std::runtime_error("timed verification failed");
            }
          }

          for (auto& measurement : measurements) {
            auto copy = input;
            run_kernel(kernels()[measurement.kernel_index].name, copy,
                       measurement.stats, true);
            measurement.verified = measurement.verified && verify(input, copy);
          }

          for (const auto& measurement : measurements) {
            const auto& kernel = kernels()[measurement.kernel_index];
            const auto padded = kernel.network ? bitonic_padded_size(n) : 0;
            const auto gates = kernel.network ? bitonic_gate_count(padded) : 0;
            std::cout << 1 << ',' << kernel.name << ',' << pattern << ',' << n << ','
                      << trial << ',' << seed << ',' << current_seed << ','
                      << fingerprint << ',' << measurement.execution_order << ','
                      << measurement.ns << ',' << measurement.stats.comparisons << ','
                      << measurement.stats.swaps << ',' << measurement.stats.writes
                      << ',' << padded << ',' << gates << ','
                      << (measurement.verified ? 1 : 0) << '\n';
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
