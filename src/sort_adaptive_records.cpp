#include "sortlab/adaptive_merge_records.hpp"

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
  MergePolicy merge_policy{};
  MinrunPolicy minrun_policy{};
  MergeKernelSpec kernel{};
};

struct Config {
  std::uint64_t seed = 0x5EEDULL;
  int trials = 11;
  int warmups = 1;
  std::vector<std::size_t> sizes = {64, 315, 4096, 65536};
  std::vector<std::size_t> payload_words = {0, 1, 3, 7, 15, 31};
  std::vector<std::string> policies;
  std::vector<std::string> minruns;
  std::vector<std::string> buffers;
  std::vector<std::string> searches;
  std::vector<std::size_t> gallop_thresholds = {4, 7, 12};
  std::vector<std::string> patterns;
};

template <std::size_t Words>
struct Measurement {
  std::size_t treatment_index{};
  std::size_t execution_order{};
  std::int64_t ns{};
  RecordStats stats{};
  RecordMergeKernelMetrics kernel_metrics{};
  AdaptiveMergeMetrics merge_metrics{};
  bool verified{};
  bool stable{};
};

MergePolicy parse_merge_policy(std::string_view name) {
  if (name == "pairwise") return MergePolicy::pairwise;
  if (name == "timsort_stack") return MergePolicy::timsort_stack;
  if (name == "powersort") return MergePolicy::powersort;
  throw std::runtime_error("unknown adaptive-record merge policy: " +
                           std::string(name));
}

MinrunPolicy parse_minrun_policy(std::string_view name) {
  if (name == "none") return MinrunPolicy::none;
  if (name == "classic") return MinrunPolicy::classic;
  if (name == "balanced") return MinrunPolicy::balanced;
  throw std::runtime_error("unknown adaptive-record minrun policy: " +
                           std::string(name));
}

MergeBufferPolicy parse_buffer_policy(std::string_view name) {
  if (name == "full") return MergeBufferPolicy::full;
  if (name == "smaller") return MergeBufferPolicy::smaller_run;
  throw std::runtime_error("unknown adaptive-record buffer policy: " +
                           std::string(name));
}

MergeSearchPolicy parse_search_policy(std::string_view name) {
  if (name == "linear") return MergeSearchPolicy::linear;
  if (name == "gallop") return MergeSearchPolicy::gallop;
  throw std::runtime_error("unknown adaptive-record search policy: " +
                           std::string(name));
}

std::string treatment_name(const Treatment& treatment) {
  return std::string(merge_policy_name(treatment.merge_policy)) + "_" +
         minrun_policy_name(treatment.minrun_policy) + "_" +
         merge_kernel_name(treatment.kernel);
}

std::vector<Treatment> make_treatments(const Config& config) {
  const auto policy_names = config.policies.empty()
      ? std::vector<std::string>{"powersort"}
      : config.policies;
  const auto minrun_names = config.minruns.empty()
      ? std::vector<std::string>{"balanced"}
      : config.minruns;
  const auto buffer_names = config.buffers.empty()
      ? std::vector<std::string>{"full", "smaller"}
      : config.buffers;
  const auto search_names = config.searches.empty()
      ? std::vector<std::string>{"linear", "gallop"}
      : config.searches;

  std::vector<Treatment> result;
  for (const auto& policy_name : policy_names) {
    const auto policy = parse_merge_policy(policy_name);
    for (const auto& minrun_name : minrun_names) {
      const auto minrun = parse_minrun_policy(minrun_name);
      for (const auto& buffer_name : buffer_names) {
        const auto buffer = parse_buffer_policy(buffer_name);
        for (const auto& search_name : search_names) {
          const auto search = parse_search_policy(search_name);
          if (search == MergeSearchPolicy::linear) {
            result.push_back({policy, minrun, {buffer, search, 0}});
          } else {
            if (config.gallop_thresholds.empty()) {
              throw std::runtime_error("gallop search requires at least one threshold");
            }
            for (const auto threshold : config.gallop_thresholds) {
              if (threshold < 2) {
                throw std::runtime_error("gallop thresholds must be at least 2");
              }
              result.push_back({policy, minrun, {buffer, search, threshold}});
            }
          }
        }
      }
    }
  }
  return result;
}

std::vector<std::string> selected_patterns(const Config& config) {
  if (config.patterns.empty()) return adaptive_record_patterns();
  for (const auto& name : config.patterns) {
    if (std::find(adaptive_record_patterns().begin(),
                  adaptive_record_patterns().end(), name) ==
        adaptive_record_patterns().end()) {
      throw std::runtime_error("unknown adaptive-record pattern: " + name);
    }
  }
  return config.patterns;
}

template <std::size_t Words>
std::vector<Value> record_keys(const std::vector<Record<Words>>& records) {
  std::vector<Value> keys;
  keys.reserve(records.size());
  for (const auto& record : records) keys.push_back(record.key);
  return keys;
}

template <std::size_t Words>
bool direct_stability_contract() {
  const std::vector<std::vector<Value>> key_sets = {
      {1, 1, 3, 5, 1, 2, 3, 6},
      {0, 1, 1, 2, 4, 1, 3, 4},
  };
  const std::vector<std::size_t> split = {4, 5};
  const std::vector<MergeKernelSpec> kernels = {
      {MergeBufferPolicy::full, MergeSearchPolicy::linear, 0},
      {MergeBufferPolicy::smaller_run, MergeSearchPolicy::linear, 0},
      {MergeBufferPolicy::full, MergeSearchPolicy::gallop, 2},
      {MergeBufferPolicy::smaller_run, MergeSearchPolicy::gallop, 2},
  };

  for (std::size_t scenario = 0; scenario < key_sets.size(); ++scenario) {
    std::vector<Record<Words>> input(key_sets[scenario].size());
    for (std::size_t i = 0; i < input.size(); ++i) {
      input[i].key = key_sets[scenario][i];
      input[i].ordinal = static_cast<std::uint64_t>(i);
      if constexpr (Words > 0) {
        for (std::size_t word = 0; word < Words; ++word) {
          input[i].payload.words[word] = splitmix64(
              static_cast<std::uint64_t>(i) ^
              (static_cast<std::uint64_t>(word) << 32U));
        }
      }
    }
    const AdaptiveRun left{0, split[scenario], 0};
    const AdaptiveRun right{split[scenario], input.size() - split[scenario], 0};
    for (const auto& kernel : kernels) {
      auto output = input;
      RecordMergeWorkspace<Words> workspace;
      RecordMergeKernelMetrics metrics;
      RecordStats stats;
      record_merge_adjacent_with_kernel<true>(output, workspace, left, right,
                                              kernel, metrics, stats);
      const auto check = verify_records(input, output);
      if (!check.correct || !check.stable) return false;
    }
  }

  std::vector<Record<Words>> gallop_input(64);
  for (std::size_t i = 0; i < 32; ++i) {
    gallop_input[i].key = static_cast<Value>(i);
    gallop_input[i].ordinal = static_cast<std::uint64_t>(i);
    gallop_input[32 + i].key = static_cast<Value>(1000 + i);
    gallop_input[32 + i].ordinal = static_cast<std::uint64_t>(32 + i);
  }
  auto output = gallop_input;
  RecordMergeWorkspace<Words> workspace;
  RecordMergeKernelMetrics metrics;
  RecordStats stats;
  const MergeKernelSpec kernel{MergeBufferPolicy::smaller_run,
                               MergeSearchPolicy::gallop, 4};
  record_merge_adjacent_with_kernel<true>(output, workspace, {0, 32, 0},
                                          {32, 32, 0}, kernel, metrics, stats);
  const auto check = verify_records(gallop_input, output);
  return check.correct && check.stable && metrics.gallop_entries > 0;
}

std::vector<Treatment> self_test_treatments() {
  std::vector<Treatment> result;
  const MergeKernelSpec full_linear{MergeBufferPolicy::full,
                                    MergeSearchPolicy::linear, 0};
  for (const auto policy : {MergePolicy::pairwise, MergePolicy::timsort_stack,
                            MergePolicy::powersort}) {
    for (const auto minrun : {MinrunPolicy::none, MinrunPolicy::classic,
                              MinrunPolicy::balanced}) {
      result.push_back({policy, minrun, full_linear});
    }
  }
  result.push_back({MergePolicy::powersort, MinrunPolicy::balanced,
                    {MergeBufferPolicy::smaller_run, MergeSearchPolicy::linear, 0}});
  for (const auto threshold : {4U, 7U}) {
    result.push_back({MergePolicy::powersort, MinrunPolicy::balanced,
                      {MergeBufferPolicy::full, MergeSearchPolicy::gallop,
                       threshold}});
    result.push_back({MergePolicy::powersort, MinrunPolicy::balanced,
                      {MergeBufferPolicy::smaller_run, MergeSearchPolicy::gallop,
                       threshold}});
  }
  return result;
}

template <std::size_t Words>
bool self_test_width(std::size_t& cases) {
  const auto treatments = self_test_treatments();
  for (const auto& pattern : adaptive_record_patterns()) {
    for (const std::size_t n : {0U, 1U, 2U, 3U, 31U, 64U, 65U}) {
      const auto seed = trial_seed(0xA11CEULL, pattern, n, 5);
      const auto input = make_adaptive_records<Words>(pattern, n, seed);
      for (const auto& treatment : treatments) {
        auto timed = input;
        RecordStats timed_stats;
        RecordMergeKernelMetrics timed_kernel_metrics;
        AdaptiveMergeMetrics timed_merge_metrics;
        adaptive_record_merge_sort<false>(
            timed, timed_stats, treatment.merge_policy, treatment.minrun_policy,
            treatment.kernel, timed_kernel_metrics, timed_merge_metrics);
        const auto timed_check = verify_records(input, timed);

        auto instrumented = input;
        RecordStats stats;
        RecordMergeKernelMetrics kernel_metrics;
        AdaptiveMergeMetrics merge_metrics;
        adaptive_record_merge_sort<true>(
            instrumented, stats, treatment.merge_policy,
            treatment.minrun_policy, treatment.kernel, kernel_metrics,
            merge_metrics);
        const auto instrumented_check = verify_records(input, instrumented);

        if (!timed_check.correct || !timed_check.stable ||
            !instrumented_check.correct || !instrumented_check.stable ||
            timed != instrumented) {
          std::cerr << "FAIL adaptive record " << treatment_name(treatment)
                    << " pattern=" << pattern << " n=" << n
                    << " payload_words=" << Words << '\n';
          return false;
        }
        if (merge_metrics.effective_runs > 0 &&
            merge_metrics.merges + 1 != merge_metrics.effective_runs) {
          std::cerr << "FAIL adaptive record merge-tree invariant\n";
          return false;
        }
        if (treatment.kernel.buffer == MergeBufferPolicy::smaller_run &&
            kernel_metrics.temp_records_peak > n / 2) {
          std::cerr << "FAIL adaptive record smaller-buffer bound\n";
          return false;
        }
        ++cases;
      }
    }
  }
  return direct_stability_contract<Words>();
}

int self_test() {
  std::size_t cases = 0;
  if (!self_test_width<0>(cases) || !self_test_width<7>(cases) ||
      !self_test_width<31>(cases)) {
    return 1;
  }
  std::cout << "PASS: adaptive records across 13 treatments, "
            << adaptive_record_patterns().size()
            << " workload families, 3 representative payload widths, " << cases
            << " deterministic treatment cases, and direct stability/gallop contracts\n";
  return 0;
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
  std::cout << "{\"schema_version\":1,\"benchmark\":\"adaptive_records\","
            << "\"compiler\":\"" << compiler << "\",\"cplusplus\":"
            << __cplusplus << ",\"workload_count\":"
            << adaptive_record_patterns().size() << ",\"record_bytes\":["
            << sizeof(Record<0>) << ',' << sizeof(Record<1>) << ','
            << sizeof(Record<3>) << ',' << sizeof(Record<7>) << ','
            << sizeof(Record<15>) << ',' << sizeof(Record<31>) << "]}\n";
}

template <std::size_t Words>
void run_width(const Config& config, const std::vector<Treatment>& treatments,
               const std::vector<std::string>& patterns) {
  for (const auto n : config.sizes) {
    for (const auto& pattern : patterns) {
      for (int warmup = 0; warmup < config.warmups; ++warmup) {
        const auto seed = trial_seed(
            config.seed ^ 0xA0761D6478BD642FULL, pattern, n,
            static_cast<std::uint64_t>(warmup));
        const auto input = make_adaptive_records<Words>(pattern, n, seed);
        for (const auto& treatment : treatments) {
          auto copy = input;
          RecordStats stats;
          RecordMergeKernelMetrics kernel_metrics;
          AdaptiveMergeMetrics merge_metrics;
          adaptive_record_merge_sort<false>(
              copy, stats, treatment.merge_policy, treatment.minrun_policy,
              treatment.kernel, kernel_metrics, merge_metrics);
          const auto check = verify_records(input, copy);
          if (!check.correct || !check.stable) {
            throw std::runtime_error("adaptive-record warmup verification failed");
          }
        }
      }

      for (int trial = 0; trial < config.trials; ++trial) {
        const auto tseed = trial_seed(config.seed, pattern, n,
                                      static_cast<std::uint64_t>(trial));
        const auto input = make_adaptive_records<Words>(pattern, n, tseed);
        const auto keys = record_keys(input);
        const auto khash = key_hash(keys);
        const auto fingerprint = record_hash(input);
        std::vector<std::size_t> order(treatments.size());
        std::iota(order.begin(), order.end(), 0);
        deterministic_shuffle(
            order, splitmix64(tseed ^ static_cast<std::uint64_t>(Words) ^
                              0x94D049BB133111EBULL));

        std::vector<Measurement<Words>> measurements;
        measurements.reserve(treatments.size());
        std::size_t execution_order = 0;
        for (const auto index : order) {
          const auto& treatment = treatments[index];
          auto copy = input;
          RecordStats stats;
          RecordMergeKernelMetrics kernel_metrics;
          AdaptiveMergeMetrics merge_metrics;
          const auto start = std::chrono::steady_clock::now();
          adaptive_record_merge_sort<false>(
              copy, stats, treatment.merge_policy, treatment.minrun_policy,
              treatment.kernel, kernel_metrics, merge_metrics);
          const auto stop = std::chrono::steady_clock::now();
          const auto check = verify_records(input, copy);
          if (!check.correct || !check.stable) {
            throw std::runtime_error("adaptive-record timed verification failed");
          }
          measurements.push_back(
              {index, execution_order++,
               std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
                   .count(),
               {}, {}, {}, true, true});
        }

        for (auto& measurement : measurements) {
          const auto& treatment = treatments[measurement.treatment_index];
          auto copy = input;
          adaptive_record_merge_sort<true>(
              copy, measurement.stats, treatment.merge_policy,
              treatment.minrun_policy, treatment.kernel,
              measurement.kernel_metrics, measurement.merge_metrics);
          const auto check = verify_records(input, copy);
          measurement.verified = measurement.verified && check.correct;
          measurement.stable = measurement.stable && check.stable;
          if (!check.correct || !check.stable) {
            throw std::runtime_error(
                "adaptive-record instrumented verification failed");
          }
        }

        for (const auto& measurement : measurements) {
          const auto& treatment = treatments[measurement.treatment_index];
          const auto& kernel = treatment.kernel;
          const auto& km = measurement.kernel_metrics;
          const auto& mm = measurement.merge_metrics;
          const auto bytes = static_cast<std::uint64_t>(sizeof(Record<Words>));
          std::cout
              << 1 << ',' << treatment_name(treatment) << ','
              << merge_policy_name(treatment.merge_policy) << ','
              << minrun_policy_name(treatment.minrun_policy) << ','
              << merge_buffer_policy_name(kernel.buffer) << ','
              << merge_search_policy_name(kernel.search) << ','
              << kernel.gallop_threshold << ',' << pattern << ',' << n << ','
              << Words << ',' << sizeof(Record<Words>) << ',' << trial << ','
              << config.seed << ',' << tseed << ',' << khash << ',' << fingerprint
              << ',' << measurement.execution_order << ',' << measurement.ns << ','
              << measurement.stats.comparisons << ',' << measurement.stats.swaps
              << ',' << measurement.stats.explicit_moves << ','
              << measurement.stats.explicit_moves * bytes << ','
              << mm.natural_runs << ',' << mm.effective_runs << ','
              << mm.reversed_runs << ',' << mm.extended_elements << ',' << mm.merges
              << ',' << mm.scheduled_merge_cost << ',' << mm.max_pending_runs << ','
              << km.temp_records_peak << ',' << km.temp_records_peak * bytes << ','
              << km.temp_capacity_peak << ',' << km.temp_capacity_peak * bytes << ','
              << km.temp_records_copied << ',' << km.temp_records_copied * bytes
              << ',' << km.gallop_entries << ',' << km.gallop_records << ','
              << (measurement.stable ? 1 : 0) << ','
              << (measurement.verified ? 1 : 0) << '\n';
        }
      }
    }
  }
}

void dispatch_width(std::size_t words, const Config& config,
                    const std::vector<Treatment>& treatments,
                    const std::vector<std::string>& patterns) {
  switch (words) {
    case 0:
      run_width<0>(config, treatments, patterns);
      break;
    case 1:
      run_width<1>(config, treatments, patterns);
      break;
    case 3:
      run_width<3>(config, treatments, patterns);
      break;
    case 7:
      run_width<7>(config, treatments, patterns);
      break;
    case 15:
      run_width<15>(config, treatments, patterns);
      break;
    case 31:
      run_width<31>(config, treatments, patterns);
      break;
    default:
      throw std::runtime_error(
          "unsupported adaptive-record payload width; choose 0,1,3,7,15,31");
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    bool do_test = false;
    bool environment = false;
    Config config;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--self-test") {
        do_test = true;
      } else if (arg == "--environment") {
        environment = true;
      } else if (arg == "--seed" && i + 1 < argc) {
        config.seed = std::stoull(argv[++i]);
      } else if (arg == "--trials" && i + 1 < argc) {
        config.trials = std::stoi(argv[++i]);
      } else if (arg == "--warmups" && i + 1 < argc) {
        config.warmups = std::stoi(argv[++i]);
      } else if (arg == "--sizes" && i + 1 < argc) {
        config.sizes = parse_sizes(argv[++i]);
      } else if (arg == "--payload-words" && i + 1 < argc) {
        config.payload_words = parse_sizes(argv[++i]);
      } else if (arg == "--policies" && i + 1 < argc) {
        config.policies = split_csv(argv[++i]);
      } else if (arg == "--minruns" && i + 1 < argc) {
        config.minruns = split_csv(argv[++i]);
      } else if (arg == "--buffers" && i + 1 < argc) {
        config.buffers = split_csv(argv[++i]);
      } else if (arg == "--searches" && i + 1 < argc) {
        config.searches = split_csv(argv[++i]);
      } else if (arg == "--gallop-thresholds" && i + 1 < argc) {
        config.gallop_thresholds = parse_sizes(argv[++i]);
      } else if (arg == "--patterns" && i + 1 < argc) {
        config.patterns = split_csv(argv[++i]);
      } else if (arg == "--help") {
        std::cout
            << "sort_adaptive_records [--self-test] [--environment]\n"
            << "  [--seed N] [--trials N] [--warmups N]\n"
            << "  [--sizes a,b] [--payload-words 0,1,3,7,15,31]\n"
            << "  [--policies pairwise,timsort_stack,powersort]\n"
            << "  [--minruns none,classic,balanced]\n"
            << "  [--buffers full,smaller] [--searches linear,gallop]\n"
            << "  [--gallop-thresholds 4,7,12] [--patterns a,b]\n";
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
    if (config.trials < 1 || config.warmups < 0 || config.sizes.empty() ||
        config.payload_words.empty()) {
      throw std::runtime_error("invalid adaptive-record benchmark configuration");
    }

    const auto treatments = make_treatments(config);
    const auto patterns = selected_patterns(config);
    std::cout
        << "schema_version,treatment,merge_policy,minrun_policy,buffer_policy,"
           "search_policy,gallop_threshold,pattern,n,payload_words,record_bytes,"
           "trial,experiment_seed,trial_seed,key_hash,input_hash,execution_order,"
           "ns,comparisons,swaps,explicit_record_moves,explicit_bytes_moved,"
           "natural_runs,effective_runs,reversed_runs,extended_elements,merges,"
           "scheduled_merge_cost,max_pending_runs,temp_records_peak,"
           "temp_bytes_requested_peak,temp_capacity_records_peak,"
           "temp_capacity_bytes_peak,temp_records_copied,temp_bytes_copied,"
           "gallop_entries,gallop_records,stable_on_trial,verified\n";
    for (const auto words : config.payload_words) {
      dispatch_width(words, config, treatments, patterns);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 2;
  }
}
