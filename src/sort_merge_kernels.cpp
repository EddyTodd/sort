#include "sortlab/adaptive_merge_kernels.hpp"
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

std::vector<MergeKernelSpec> specs(const std::vector<std::size_t>& thresholds) {
  std::vector<MergeKernelSpec> out;
  for (const auto buffer : {MergeBufferPolicy::full, MergeBufferPolicy::smaller_run}) {
    out.push_back({buffer, MergeSearchPolicy::linear, 0});
    for (const auto threshold : thresholds) {
      if (threshold < 2) throw std::runtime_error("gallop threshold must be >= 2");
      out.push_back({buffer, MergeSearchPolicy::gallop, threshold});
    }
  }
  return out;
}

std::vector<std::string> selected_kernel_patterns(const std::vector<std::string>& names) {
  if (names.empty()) return merge_policy_patterns();
  for (const auto& name : names) {
    if (std::find(merge_policy_patterns().begin(), merge_policy_patterns().end(), name) ==
        merge_policy_patterns().end()) {
      throw std::runtime_error("unknown pattern: " + name);
    }
  }
  return names;
}

int self_test() {
  const auto treatments = specs({4, 7, 12});
  std::size_t cases = 0;
  for (const auto& spec : treatments) {
    for (const auto& pattern : merge_policy_patterns()) {
      for (const std::size_t n : {0U, 1U, 2U, 3U, 7U, 31U, 32U, 33U,
                                  63U, 64U, 65U, 127U, 315U, 1024U}) {
        const auto seed = trial_seed(0xA11CEULL, pattern, n, 11);
        const auto input = make_merge_policy_data(pattern, n, seed);

        auto timed = input;
        Stats timed_stats;
        MergeKernelMetrics timed_kernel_metrics;
        AdaptiveMergeMetrics timed_merge_metrics;
        powersort_balanced_with_kernel<false>(timed, timed_stats, spec,
                                              timed_kernel_metrics, timed_merge_metrics);

        auto instrumented = input;
        Stats stats;
        MergeKernelMetrics kernel_metrics;
        AdaptiveMergeMetrics merge_metrics;
        powersort_balanced_with_kernel<true>(instrumented, stats, spec,
                                             kernel_metrics, merge_metrics);

        if (!verify(input, timed) || !verify(input, instrumented) || timed != instrumented) {
          std::cerr << "FAIL " << merge_kernel_name(spec) << ' ' << pattern
                    << " n=" << n << '\n';
          return 1;
        }
        if (n > 1 && merge_metrics.effective_runs > 0 &&
            merge_metrics.merges + 1 != merge_metrics.effective_runs) {
          std::cerr << "FAIL schedule tree invariant\n";
          return 1;
        }
        if (spec.buffer == MergeBufferPolicy::smaller_run &&
            kernel_metrics.temp_elements_peak > n / 2) {
          std::cerr << "FAIL smaller-run buffer bound\n";
          return 1;
        }
        ++cases;
      }
    }
  }

  // Force two already-sorted runs to remain distinct so the gallop path is
  // exercised independently of natural-run discovery.
  {
    std::vector<Value> values;
    for (int i = 0; i < 32; ++i) values.push_back(i);
    for (int i = 0; i < 32; ++i) values.push_back(100 + i);
    Stats stats;
    MergeKernelMetrics metrics;
    MergeKernelWorkspace workspace;
    const MergeKernelSpec gallop{MergeBufferPolicy::full, MergeSearchPolicy::gallop, 4};
    merge_adjacent_with_kernel<true>(values, workspace, {0, 32, 0}, {32, 32, 0},
                                     gallop, metrics, stats);
    if (!std::is_sorted(values.begin(), values.end()) || metrics.gallop_entries == 0 ||
        metrics.gallop_elements == 0) {
      std::cerr << "FAIL gallop activation contract\n";
      return 1;
    }
  }

  std::cout << "PASS: " << treatments.size() << " merge-kernel treatments, "
            << merge_policy_patterns().size() << " workloads, " << cases
            << " deterministic cases\n";
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
    std::vector<std::size_t> sizes = {64, 315, 2048, 16384, 131072};
    std::vector<std::size_t> thresholds = {4, 7, 12, 16};
    std::vector<std::string> pattern_names;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--self-test") do_test = true;
      else if (arg == "--environment") environment = true;
      else if (arg == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
      else if (arg == "--trials" && i + 1 < argc) trials = std::stoi(argv[++i]);
      else if (arg == "--warmups" && i + 1 < argc) warmups = std::stoi(argv[++i]);
      else if (arg == "--sizes" && i + 1 < argc) sizes = parse_sizes(argv[++i]);
      else if (arg == "--gallop-thresholds" && i + 1 < argc) thresholds = parse_sizes(argv[++i]);
      else if (arg == "--patterns" && i + 1 < argc) pattern_names = split_csv(argv[++i]);
      else if (arg == "--help") {
        std::cout << "sort_merge_kernels [--self-test] [--environment] [--seed N] "
                     "[--trials N] [--warmups N] [--sizes a,b] "
                     "[--gallop-thresholds a,b] [--patterns a,b]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown/incomplete argument: " + arg);
      }
    }

    if (do_test) return self_test();
    if (environment) {
      std::cout << "{\"schema_version\":1,\"benchmark\":\"merge_kernels\","
                   "\"scheduler\":\"powersort\",\"minrun\":\"balanced\","
                << "\"value_bits\":" << sizeof(Value) * 8 << "}\n";
      return 0;
    }
    if (trials < 1 || warmups < 0 || sizes.empty() || thresholds.empty()) {
      throw std::runtime_error("invalid configuration");
    }

    const auto treatments = specs(thresholds);
    const auto patterns = selected_kernel_patterns(pattern_names);

    std::cout << "schema_version,treatment,buffer_policy,search_policy,gallop_threshold,"
                 "pattern,n,trial,experiment_seed,trial_seed,input_hash,execution_order,ns,"
                 "comparisons,swaps,writes,natural_runs,effective_runs,merges,scheduled_merge_cost,"
                 "max_pending_runs,temp_elements_peak,temp_capacity_peak,temp_bytes_peak,"
                 "temp_elements_copied,gallop_entries,gallop_elements,verified\n";

    for (const auto n : sizes) {
      for (const auto& pattern : patterns) {
        for (int warmup = 0; warmup < warmups; ++warmup) {
          const auto warm_seed = trial_seed(seed ^ 0xA0761D6478BD642FULL, pattern, n,
                                            static_cast<std::uint64_t>(warmup));
          const auto input = make_merge_policy_data(pattern, n, warm_seed);
          for (const auto& spec : treatments) {
            auto copy = input;
            Stats stats;
            MergeKernelMetrics kernel_metrics;
            AdaptiveMergeMetrics merge_metrics;
            powersort_balanced_with_kernel<false>(copy, stats, spec, kernel_metrics,
                                                  merge_metrics);
            if (!verify(input, copy)) throw std::runtime_error("warmup verification failed");
          }
        }

        for (int trial = 0; trial < trials; ++trial) {
          const auto trial_value = trial_seed(seed, pattern, n, static_cast<std::uint64_t>(trial));
          const auto input = make_merge_policy_data(pattern, n, trial_value);
          const auto fingerprint = input_hash(input);
          std::vector<std::size_t> order(treatments.size());
          std::iota(order.begin(), order.end(), 0);
          deterministic_shuffle(order, splitmix64(trial_value ^ 0xD1B54A32D192ED03ULL));

          struct Measurement {
            std::size_t treatment_index{};
            std::size_t execution_order{};
            std::int64_t ns{};
            Stats stats{};
            MergeKernelMetrics kernel_metrics{};
            AdaptiveMergeMetrics merge_metrics{};
            bool verified{};
          };
          std::vector<Measurement> measurements;
          std::size_t execution_order = 0;

          for (const auto index : order) {
            auto copy = input;
            Stats stats;
            MergeKernelMetrics kernel_metrics;
            AdaptiveMergeMetrics merge_metrics;
            const auto start = std::chrono::steady_clock::now();
            powersort_balanced_with_kernel<false>(copy, stats, treatments[index],
                                                  kernel_metrics, merge_metrics);
            const auto stop = std::chrono::steady_clock::now();
            const bool ok = verify(input, copy);
            if (!ok) throw std::runtime_error("timed verification failed");
            measurements.push_back({
                index,
                execution_order++,
                std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count(),
                {}, {}, {}, ok});
          }

          for (auto& measurement : measurements) {
            auto copy = input;
            powersort_balanced_with_kernel<true>(
                copy, measurement.stats, treatments[measurement.treatment_index],
                measurement.kernel_metrics, measurement.merge_metrics);
            measurement.verified = measurement.verified && verify(input, copy);
            if (!measurement.verified) throw std::runtime_error("instrumented verification failed");
          }

          for (const auto& measurement : measurements) {
            const auto& spec = treatments[measurement.treatment_index];
            const auto& km = measurement.kernel_metrics;
            const auto& mm = measurement.merge_metrics;
            std::cout << 1 << ',' << merge_kernel_name(spec) << ','
                      << merge_buffer_policy_name(spec.buffer) << ','
                      << merge_search_policy_name(spec.search) << ','
                      << spec.gallop_threshold << ',' << pattern << ',' << n << ',' << trial << ','
                      << seed << ',' << trial_value << ',' << fingerprint << ','
                      << measurement.execution_order << ',' << measurement.ns << ','
                      << measurement.stats.comparisons << ',' << measurement.stats.swaps << ','
                      << measurement.stats.writes << ',' << mm.natural_runs << ','
                      << mm.effective_runs << ',' << mm.merges << ',' << mm.scheduled_merge_cost << ','
                      << mm.max_pending_runs << ',' << km.temp_elements_peak << ','
                      << km.temp_capacity_peak << ',' << km.temp_capacity_peak * sizeof(Value) << ','
                      << km.temp_elements_copied << ',' << km.gallop_entries << ','
                      << km.gallop_elements << ',' << (measurement.verified ? 1 : 0) << '\n';
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
