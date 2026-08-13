#include "sortlab/leaf_hybrids.hpp"
#include "sortlab/workloads.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace sortlab;

namespace {

TinyKernel parse_kernel(const std::string& name) {
  if (name == "insertion") return TinyKernel::insertion;
  if (name == "binary_insertion") return TinyKernel::binary_insertion;
  if (name == "bitonic_network") return TinyKernel::bitonic_network;
  throw std::runtime_error("unknown kernel: " + name);
}

void run_configuration(const std::string& family, TinyKernel kernel,
                       std::size_t cutoff, std::vector<Value>& values,
                       Stats& stats, bool count) {
  if (family == "merge") {
    if (count) {
      merge_leaf_sort<true>(values, cutoff, kernel, stats);
    } else {
      merge_leaf_sort<false>(values, cutoff, kernel, stats);
    }
    return;
  }
  if (family == "quick") {
    if (count) {
      quick_leaf_sort<true>(values, cutoff, kernel, stats);
    } else {
      quick_leaf_sort<false>(values, cutoff, kernel, stats);
    }
    return;
  }
  if (family == "intro") {
    if (count) {
      intro_leaf_sort<true>(values, cutoff, kernel, stats);
    } else {
      intro_leaf_sort<false>(values, cutoff, kernel, stats);
    }
    return;
  }
  throw std::runtime_error("unknown family: " + family);
}

int self_test() {
  std::size_t cases = 0;
  for (const auto& pattern : all_patterns()) {
    for (const auto n : {0U, 1U, 2U, 3U, 17U, 64U, 127U}) {
      const auto input = make_data(pattern, n, trial_seed(0xA11CE, pattern, n, 5));
      for (const auto* family : {"merge", "quick", "intro"}) {
        for (const auto kernel : {TinyKernel::insertion,
                                  TinyKernel::binary_insertion,
                                  TinyKernel::bitonic_network}) {
          for (const auto cutoff : {4U, 8U, 16U, 24U, 32U}) {
            auto copy = input;
            Stats unused;
            run_configuration(family, kernel, cutoff, copy, unused, false);
            if (!verify(input, copy)) {
              std::cerr << "FAIL " << family << '/' << tiny_kernel_name(kernel)
                        << " cutoff=" << cutoff << " n=" << n << '\n';
              return 1;
            }
          }
        }
      }
      ++cases;
    }
  }
  std::cout << "PASS: 3 hybrid families x 3 leaf kernels x 5 cutoffs, "
            << all_patterns().size() << " workloads, " << cases
            << " deterministic cases\n";
  return 0;
}

struct Configuration {
  std::string family;
  TinyKernel kernel;
  std::size_t cutoff;
};

struct Measurement {
  std::size_t config_index{};
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
    int trials = 51;
    int warmups = 2;
    std::vector<std::size_t> sizes = {64, 128, 256, 512, 1024,
                                      2048, 4096, 8192, 16384};
    std::vector<std::size_t> cutoffs = {4, 8, 12, 16, 24, 32};
    std::vector<std::string> families = {"merge", "quick", "intro"};
    std::vector<std::string> kernel_names = {
        "insertion", "binary_insertion", "bitonic_network"};
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
      } else if (arg == "--cutoffs" && i + 1 < argc) {
        cutoffs = parse_sizes(argv[++i]);
      } else if (arg == "--families" && i + 1 < argc) {
        families = split_csv(argv[++i]);
      } else if (arg == "--kernels" && i + 1 < argc) {
        kernel_names = split_csv(argv[++i]);
      } else if (arg == "--patterns" && i + 1 < argc) {
        pattern_names = split_csv(argv[++i]);
      } else if (arg == "--help") {
        std::cout << "sort_leaf_hybrids [--self-test] [--environment] [--seed N] "
                     "[--trials N] [--warmups N] [--sizes a,b] [--cutoffs a,b] "
                     "[--families a,b] [--kernels a,b] [--patterns a,b]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown or incomplete argument: " + arg);
      }
    }

    if (do_test) return self_test();
    if (environment) {
      std::cout << "{\"schema_version\":1,\"families\":3,"
                   "\"leaf_kernels\":3,\"max_network_cutoff\":32}\n";
      return 0;
    }
    if (trials < 1 || warmups < 0) {
      throw std::runtime_error("invalid trial/warmup count");
    }

    std::vector<TinyKernel> kernels;
    for (const auto& name : kernel_names) kernels.push_back(parse_kernel(name));
    for (const auto& family : families) {
      if (family != "merge" && family != "quick" && family != "intro") {
        throw std::runtime_error("unknown family: " + family);
      }
    }
    for (const auto kernel : kernels) {
      for (const auto cutoff : cutoffs) validate_leaf_configuration(kernel, cutoff);
    }

    const auto patterns = selected_patterns(pattern_names);
    std::vector<Configuration> configs;
    for (const auto& family : families) {
      for (const auto kernel : kernels) {
        for (const auto cutoff : cutoffs) {
          configs.push_back({family, kernel, cutoff});
        }
      }
    }

    std::cout << "schema_version,family,kernel,cutoff,pattern,n,trial,"
                 "experiment_seed,trial_seed,input_hash,execution_order,ns,"
                 "comparisons,swaps,writes,verified\n";

    for (const auto n : sizes) {
      for (const auto& pattern : patterns) {
        for (int warmup = 0; warmup < warmups; ++warmup) {
          const auto warmup_seed =
              trial_seed(seed ^ 0xE7037ED1A0B428DBULL, pattern, n,
                         static_cast<std::uint64_t>(warmup));
          const auto input = make_data(pattern, n, warmup_seed);
          for (const auto& config : configs) {
            auto copy = input;
            Stats unused;
            run_configuration(config.family, config.kernel, config.cutoff, copy,
                              unused, false);
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
          std::vector<std::size_t> order(configs.size());
          for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
          deterministic_shuffle(order,
                                splitmix64(current_seed ^ 0x8EBC6AF09C88C6E3ULL));

          std::vector<Measurement> measurements;
          measurements.reserve(order.size());
          std::size_t execution_order = 0;
          for (const auto index : order) {
            const auto& config = configs[index];
            auto copy = input;
            Stats unused;
            const auto start = std::chrono::steady_clock::now();
            run_configuration(config.family, config.kernel, config.cutoff, copy,
                              unused, false);
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
            const auto& config = configs[measurement.config_index];
            auto copy = input;
            run_configuration(config.family, config.kernel, config.cutoff, copy,
                              measurement.stats, true);
            measurement.verified = measurement.verified && verify(input, copy);
          }

          for (const auto& measurement : measurements) {
            const auto& config = configs[measurement.config_index];
            std::cout << 1 << ',' << config.family << ','
                      << tiny_kernel_name(config.kernel) << ',' << config.cutoff
                      << ',' << pattern << ',' << n << ',' << trial << ',' << seed
                      << ',' << current_seed << ',' << fingerprint << ','
                      << measurement.execution_order << ',' << measurement.ns << ','
                      << measurement.stats.comparisons << ','
                      << measurement.stats.swaps << ',' << measurement.stats.writes
                      << ',' << (measurement.verified ? 1 : 0) << '\n';
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
