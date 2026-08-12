#include "sortlab/adaptive_merge.hpp"
#include "sortlab/run_workloads.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace sortlab;

namespace {

struct Treatment {
  MergePolicy merge_policy;
  MinrunPolicy minrun_policy;
};

struct Measurement {
  std::size_t treatment_index{};
  std::size_t execution_order{};
  std::int64_t ns{};
  Stats stats{};
  AdaptiveMergeMetrics metrics{};
  bool verified{};
};

MergePolicy parse_merge_policy(std::string_view name) {
  if (name == "pairwise") return MergePolicy::pairwise;
  if (name == "timsort_stack") return MergePolicy::timsort_stack;
  if (name == "powersort") return MergePolicy::powersort;
  throw std::runtime_error("unknown merge policy: " + std::string(name));
}

MinrunPolicy parse_minrun_policy(std::string_view name) {
  if (name == "none") return MinrunPolicy::none;
  if (name == "classic") return MinrunPolicy::classic;
  if (name == "balanced") return MinrunPolicy::balanced;
  throw std::runtime_error("unknown minrun policy: " + std::string(name));
}

std::vector<Treatment> treatments(const std::vector<std::string>& merge_names,
                                  const std::vector<std::string>& minrun_names) {
  const std::vector<std::string> merges = merge_names.empty()
      ? std::vector<std::string>{"pairwise", "timsort_stack", "powersort"}
      : merge_names;
  const std::vector<std::string> minruns = minrun_names.empty()
      ? std::vector<std::string>{"none", "classic", "balanced"}
      : minrun_names;
  std::vector<Treatment> result;
  for (const auto& merge : merges) {
    for (const auto& minrun : minruns) {
      result.push_back({parse_merge_policy(merge), parse_minrun_policy(minrun)});
    }
  }
  return result;
}

std::vector<std::string> selected_patterns(const std::vector<std::string>& names) {
  if (names.empty()) return merge_policy_patterns();
  for (const auto& name : names) {
    if (std::find(merge_policy_patterns().begin(), merge_policy_patterns().end(), name) ==
        merge_policy_patterns().end()) {
      throw std::runtime_error("unknown merge-policy pattern: " + name);
    }
  }
  return names;
}

void print_environment() {
  std::string compiler = "unknown";
#ifdef __clang__
  compiler = std::string("clang-") + __clang_version__;
#elif defined(__GNUC__)
  compiler = std::string("gcc-") + __VERSION__;
#elif defined(_MSC_VER)
  compiler = std::string("msvc-") + std::to_string(_MSC_VER);
#endif
  std::cout << "{\"schema_version\":1,\"benchmark\":\"merge_policies\","
            << "\"compiler\":\"" << compiler << "\",\"cplusplus\":" << __cplusplus
            << ",\"value_bits\":" << sizeof(Value) * 8
            << ",\"merge_policy_count\":3,\"minrun_policy_count\":3,"
            << "\"workload_count\":" << merge_policy_patterns().size() << "}\n";
}

int self_test() {
  if (classic_minrun(64) != 32 || classic_minrun(63) != 63 || classic_minrun(65) != 33) {
    std::cerr << "FAIL classic minrun contract\n";
    return 1;
  }
  BalancedMinrunState balanced(315);
  const std::vector<std::size_t> expected = {39, 39, 40, 39, 39, 40, 39, 40};
  for (const auto value : expected) {
    if (balanced.next() != value) {
      std::cerr << "FAIL balanced minrun sequence\n";
      return 1;
    }
  }

  const auto all = treatments({}, {});
  std::size_t cases = 0;
  for (const auto& treatment : all) {
    for (const auto& pattern : merge_policy_patterns()) {
      for (const std::size_t n : {0U, 1U, 2U, 3U, 7U, 31U, 32U, 33U, 63U,
                                  64U, 65U, 127U, 315U, 1024U}) {
        const auto seed = trial_seed(0x12345678ULL, pattern, n, 7);
        const auto input = make_merge_policy_data(pattern, n, seed);
        auto timed = input;
        Stats timed_stats;
        AdaptiveMergeMetrics timed_metrics;
        adaptive_merge_sort<false>(timed, timed_stats, treatment.merge_policy,
                                   treatment.minrun_policy, timed_metrics);
        auto instrumented = input;
        Stats stats;
        AdaptiveMergeMetrics metrics;
        adaptive_merge_sort<true>(instrumented, stats, treatment.merge_policy,
                                  treatment.minrun_policy, metrics);
        if (!verify(input, timed) || !verify(input, instrumented) || timed != instrumented) {
          std::cerr << "FAIL " << merge_policy_name(treatment.merge_policy) << '/'
                    << minrun_policy_name(treatment.minrun_policy) << ' ' << pattern
                    << " n=" << n << '\n';
          return 1;
        }
        if (n > 1 && metrics.effective_runs > 0 &&
            metrics.merges + 1 != metrics.effective_runs) {
          std::cerr << "FAIL merge-count tree invariant\n";
          return 1;
        }
        ++cases;
      }
    }
  }

  std::cout << "PASS: 9 policy combinations, " << merge_policy_patterns().size()
            << " workload families, " << cases << " deterministic cases\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    bool do_test = false;
    bool environment = false;
    std::uint64_t seed = 0x5EEDULL;
    int trials = 11;
    int warmups = 1;
    std::vector<std::size_t> sizes = {32, 256, 2048, 16384, 131072};
    std::vector<std::string> merge_names;
    std::vector<std::string> minrun_names;
    std::vector<std::string> pattern_names;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--self-test") do_test = true;
      else if (arg == "--environment") environment = true;
      else if (arg == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
      else if (arg == "--trials" && i + 1 < argc) trials = std::stoi(argv[++i]);
      else if (arg == "--warmups" && i + 1 < argc) warmups = std::stoi(argv[++i]);
      else if (arg == "--sizes" && i + 1 < argc) sizes = parse_sizes(argv[++i]);
      else if (arg == "--policies" && i + 1 < argc) merge_names = split_csv(argv[++i]);
      else if (arg == "--minruns" && i + 1 < argc) minrun_names = split_csv(argv[++i]);
      else if (arg == "--patterns" && i + 1 < argc) pattern_names = split_csv(argv[++i]);
      else if (arg == "--help") {
        std::cout << "sort_merge_policies [--self-test] [--environment]\n"
                  << "  [--seed N] [--trials N] [--warmups N] [--sizes a,b,c]\n"
                  << "  [--policies pairwise,timsort_stack,powersort]\n"
                  << "  [--minruns none,classic,balanced] [--patterns a,b]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown or incomplete argument: " + arg);
      }
    }

    if (do_test) return self_test();
    if (environment) {
      print_environment();
      return 0;
    }
    if (trials < 1 || warmups < 0 || sizes.empty()) {
      throw std::runtime_error("invalid trial/warmup/size configuration");
    }

    const auto configs = treatments(merge_names, minrun_names);
    const auto patterns = selected_patterns(pattern_names);

    std::cout << "schema_version,merge_policy,minrun_policy,pattern,n,trial,"
              << "experiment_seed,trial_seed,input_hash,raw_natural_runs,raw_run_entropy_bits,"
              << "execution_order,ns,comparisons,swaps,writes,natural_runs,effective_runs,reversed_runs,"
              << "extended_elements,merges,scheduled_merge_cost,max_pending_runs,"
              << "run_entropy_bits,verified\n";

    for (const auto n : sizes) {
      for (const auto& pattern : patterns) {
        for (int warmup = 0; warmup < warmups; ++warmup) {
          const auto warm_seed = trial_seed(seed ^ 0xA0761D6478BD642FULL, pattern, n,
                                            static_cast<std::uint64_t>(warmup));
          const auto input = make_merge_policy_data(pattern, n, warm_seed);
          for (const auto& config : configs) {
            auto copy = input;
            Stats stats;
            AdaptiveMergeMetrics metrics;
            adaptive_merge_sort<false>(copy, stats, config.merge_policy,
                                       config.minrun_policy, metrics);
            if (!verify(input, copy)) throw std::runtime_error("warmup verification failed");
          }
        }

        for (int trial = 0; trial < trials; ++trial) {
          const auto current_seed = trial_seed(seed, pattern, n,
                                               static_cast<std::uint64_t>(trial));
          const auto input = make_merge_policy_data(pattern, n, current_seed);
          const auto fingerprint = input_hash(input);
          const auto raw_runs = natural_run_lengths(input);
          const auto raw_entropy = run_length_entropy_bits(raw_runs, n);
          std::vector<std::size_t> order(configs.size());
          std::iota(order.begin(), order.end(), 0);
          deterministic_shuffle(order, splitmix64(current_seed ^ 0xD1B54A32D192ED03ULL));

          std::vector<Measurement> measurements;
          measurements.reserve(configs.size());
          std::size_t execution_order = 0;
          for (const auto index : order) {
            const auto& config = configs[index];
            auto copy = input;
            Stats stats;
            AdaptiveMergeMetrics metrics;
            const auto start = std::chrono::steady_clock::now();
            adaptive_merge_sort<false>(copy, stats, config.merge_policy,
                                       config.minrun_policy, metrics);
            const auto stop = std::chrono::steady_clock::now();
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            const bool ok = verify(input, copy);
            if (!ok) throw std::runtime_error("timed verification failed");
            measurements.push_back({index, execution_order++, ns, {}, {}, ok});
          }

          for (auto& measurement : measurements) {
            const auto& config = configs[measurement.treatment_index];
            auto copy = input;
            adaptive_merge_sort<true>(copy, measurement.stats, config.merge_policy,
                                      config.minrun_policy, measurement.metrics);
            const bool ok = verify(input, copy);
            measurement.verified = measurement.verified && ok;
            if (!ok) throw std::runtime_error("instrumented verification failed");
          }

          for (const auto& measurement : measurements) {
            const auto& config = configs[measurement.treatment_index];
            const auto& m = measurement.metrics;
            std::cout << 1 << ',' << merge_policy_name(config.merge_policy) << ','
                      << minrun_policy_name(config.minrun_policy) << ',' << pattern << ','
                      << n << ',' << trial << ',' << seed << ',' << current_seed << ','
                      << fingerprint << ',' << raw_runs.size() << ',' << raw_entropy << ','
                      << measurement.execution_order << ',' << measurement.ns << ','
                      << measurement.stats.comparisons << ',' << measurement.stats.swaps << ','
                      << measurement.stats.writes << ',' << m.natural_runs << ','
                      << m.effective_runs << ',' << m.reversed_runs << ','
                      << m.extended_elements << ',' << m.merges << ','
                      << m.scheduled_merge_cost << ',' << m.max_pending_runs << ','
                      << m.run_entropy_bits << ',' << (measurement.verified ? 1 : 0) << '\n';
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
