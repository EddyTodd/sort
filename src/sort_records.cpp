#include "sortlab/record_algorithms.hpp"
#include "sortlab/record_workloads.hpp"
#include "sortlab/workloads.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

using namespace sortlab;

struct RecordConfig {
  std::uint64_t seed = 0x5EEDULL;
  int trials = 11;
  int warmups = 1;
  std::size_t quadratic_limit = 8192;
  std::vector<std::size_t> sizes = {32, 256, 2048, 16384, 131072};
  std::vector<std::size_t> payload_words = {0, 1, 3, 7, 15, 31};
  std::vector<std::string> algorithm_names;
  std::vector<std::string> pattern_names;
};

template <std::size_t Words>
static std::vector<std::size_t> selected_record_algorithms(const std::vector<std::string>& names) {
  const auto& table = record_algorithms<Words>();
  std::vector<std::size_t> indexes;
  if (names.empty()) {
    indexes.resize(table.size());
    std::iota(indexes.begin(), indexes.end(), 0);
    return indexes;
  }
  for (const auto& name : names) {
    const auto it = std::find_if(table.begin(), table.end(), [&](const auto& alg) { return alg.name == name; });
    if (it == table.end()) throw std::runtime_error("unknown record algorithm: " + name);
    indexes.push_back(static_cast<std::size_t>(std::distance(table.begin(), it)));
  }
  return indexes;
}

template <std::size_t Words>
static bool record_self_test_width() {
  const auto& table = record_algorithms<Words>();
  for (const auto& pattern : all_record_patterns()) {
    for (const std::size_t n : {0U, 1U, 2U, 3U, 17U, 64U}) {
      const auto seed = trial_seed(1234567, pattern, n, 5);
      const auto input = make_records<Words>(pattern, n, seed);
      for (const auto& alg : table) {
        auto timed = input;
        RecordStats unused;
        alg.timed(timed, unused);
        const auto timed_check = verify_records(input, timed);
        if (!timed_check.correct) {
          std::cerr << "FAIL record timed " << alg.name << " pattern=" << pattern
                    << " n=" << n << " payload_words=" << Words << '\n';
          return false;
        }
        auto instrumented = input;
        RecordStats stats;
        alg.instrumented(instrumented, stats);
        const auto instrumented_check = verify_records(input, instrumented);
        if (!instrumented_check.correct || instrumented != timed) {
          std::cerr << "FAIL record instrumented " << alg.name << " pattern=" << pattern
                    << " n=" << n << " payload_words=" << Words << '\n';
          return false;
        }
        if (alg.stable_guarantee == "yes" && (!timed_check.stable || !instrumented_check.stable)) {
          std::cerr << "FAIL stable guarantee " << alg.name << " pattern=" << pattern
                    << " n=" << n << " payload_words=" << Words << '\n';
          return false;
        }
      }
    }
  }
  return true;
}

static int self_test() {
  if (!record_self_test_width<0>() || !record_self_test_width<1>() || !record_self_test_width<7>() ||
      !record_self_test_width<31>()) return 1;
  static_assert(sizeof(Record<0>) <= sizeof(Record<1>));
  static_assert(sizeof(Record<1>) < sizeof(Record<7>));
  static_assert(sizeof(Record<7>) < sizeof(Record<31>));
  std::cout << "PASS: record sorting across 9 algorithms, " << all_record_patterns().size()
            << " workload families, and representative record widths\n";
  return 0;
}

static void print_record_algorithms() {
  for (const auto& alg : record_algorithms<0>()) {
    std::cout << alg.name << ',' << alg.family << ',' << alg.stable_guarantee << ',' << alg.auxiliary << '\n';
  }
}

static void print_record_environment() {
  std::string compiler = "unknown";
#ifdef __clang__
  compiler = std::string("clang-") + __clang_version__;
#elif defined(__GNUC__)
  compiler = std::string("gcc-") + __VERSION__;
#elif defined(_MSC_VER)
  compiler = std::string("msvc-") + std::to_string(_MSC_VER);
#endif
#ifdef NDEBUG
  constexpr const char* assertions = "false";
#else
  constexpr const char* assertions = "true";
#endif
  std::cout << "{\"schema_version\":1,\"benchmark\":\"records\",\"compiler\":\"" << compiler
            << "\",\"cplusplus\":" << __cplusplus
            << ",\"pointer_bits\":" << (sizeof(void*) * 8)
            << ",\"steady_clock\":" << (std::chrono::steady_clock::is_steady ? "true" : "false")
            << ",\"assertions_enabled\":" << assertions
            << ",\"record_bytes\":[" << sizeof(Record<0>) << ',' << sizeof(Record<1>) << ','
            << sizeof(Record<3>) << ',' << sizeof(Record<7>) << ',' << sizeof(Record<15>) << ','
            << sizeof(Record<31>) << "]}\n";
}

static void print_payloads() {
  std::cout << "payload_words,record_bytes\n";
  std::cout << "0," << sizeof(Record<0>) << '\n';
  std::cout << "1," << sizeof(Record<1>) << '\n';
  std::cout << "3," << sizeof(Record<3>) << '\n';
  std::cout << "7," << sizeof(Record<7>) << '\n';
  std::cout << "15," << sizeof(Record<15>) << '\n';
  std::cout << "31," << sizeof(Record<31>) << '\n';
}

template <std::size_t Words>
struct RecordMeasurement {
  std::size_t algorithm_index{};
  std::size_t execution_order{};
  std::int64_t ns{};
  RecordStats stats{};
  bool verified{};
  bool stable{};
};

template <std::size_t Words>
static void run_width(const RecordConfig& config, const std::vector<std::string>& patterns) {
  const auto selected = selected_record_algorithms<Words>(config.algorithm_names);
  const auto& table = record_algorithms<Words>();

  for (const auto n : config.sizes) {
    for (const auto& pattern : patterns) {
      for (int w = 0; w < config.warmups; ++w) {
        const auto seed = trial_seed(config.seed ^ 0xA0761D6478BD642FULL, pattern, n,
                                     static_cast<std::uint64_t>(w));
        const auto input = make_records<Words>(pattern, n, seed);
        for (const auto idx : selected) {
          const auto& alg = table[idx];
          if (alg.quadratic && n > config.quadratic_limit) continue;
          auto copy = input;
          RecordStats unused;
          alg.timed(copy, unused);
          if (!verify_records(input, copy).correct) {
            throw std::runtime_error("record warmup verification failed for " + std::string(alg.name));
          }
        }
      }

      for (int trial = 0; trial < config.trials; ++trial) {
        const auto tseed = trial_seed(config.seed, pattern, n, static_cast<std::uint64_t>(trial));
        const auto keys = make_record_keys(pattern, n, tseed);
        const auto khash = key_hash(keys);
        const auto input = make_records<Words>(pattern, n, tseed);
        const auto fingerprint = record_hash(input);
        auto order = selected;
        deterministic_shuffle(order, splitmix64(tseed ^ static_cast<std::uint64_t>(Words) ^ 0xE7037ED1A0B428DBULL));
        std::vector<RecordMeasurement<Words>> measurements;
        measurements.reserve(order.size());

        std::size_t executed = 0;
        for (const auto idx : order) {
          const auto& alg = table[idx];
          if (alg.quadratic && n > config.quadratic_limit) continue;
          auto copy = input;
          RecordStats unused;
          const auto start = std::chrono::steady_clock::now();
          alg.timed(copy, unused);
          const auto stop = std::chrono::steady_clock::now();
          const auto check = verify_records(input, copy);
          if (!check.correct) throw std::runtime_error("record timed verification failed for " + std::string(alg.name));
          measurements.push_back({idx, executed++,
              std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count(), {}, true, check.stable});
        }

        for (auto& measurement : measurements) {
          const auto& alg = table[measurement.algorithm_index];
          auto copy = input;
          alg.instrumented(copy, measurement.stats);
          const auto check = verify_records(input, copy);
          measurement.verified = measurement.verified && check.correct;
          measurement.stable = measurement.stable && check.stable;
          if (!check.correct) throw std::runtime_error("record instrumented verification failed for " + std::string(alg.name));
        }

        for (const auto& measurement : measurements) {
          const auto& alg = table[measurement.algorithm_index];
          std::cout << 1 << ',' << alg.name << ',' << pattern << ',' << n << ',' << Words << ','
                    << sizeof(Record<Words>) << ',' << trial << ',' << config.seed << ',' << tseed << ','
                    << khash << ',' << fingerprint << ',' << measurement.execution_order << ',' << measurement.ns << ','
                    << measurement.stats.comparisons << ',' << measurement.stats.swaps << ','
                    << measurement.stats.explicit_moves << ','
                    << (measurement.stats.explicit_moves * static_cast<std::uint64_t>(sizeof(Record<Words>))) << ','
                    << (measurement.stable ? 1 : 0) << ',' << (measurement.verified ? 1 : 0) << '\n';
        }
      }
    }
  }
}

static void dispatch_width(std::size_t words, const RecordConfig& config, const std::vector<std::string>& patterns) {
  switch (words) {
    case 0: run_width<0>(config, patterns); break;
    case 1: run_width<1>(config, patterns); break;
    case 3: run_width<3>(config, patterns); break;
    case 7: run_width<7>(config, patterns); break;
    case 15: run_width<15>(config, patterns); break;
    case 31: run_width<31>(config, patterns); break;
    default: throw std::runtime_error("unsupported payload width; choose 0,1,3,7,15,31 words");
  }
}

int main(int argc, char** argv) {
  try {
    bool do_test = false;
    bool list_algorithms = false;
    bool list_payloads = false;
    bool environment = false;
    RecordConfig config;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--self-test") do_test = true;
      else if (arg == "--list-algorithms") list_algorithms = true;
      else if (arg == "--list-payloads") list_payloads = true;
      else if (arg == "--environment") environment = true;
      else if (arg == "--seed" && i + 1 < argc) config.seed = std::stoull(argv[++i]);
      else if (arg == "--trials" && i + 1 < argc) config.trials = std::stoi(argv[++i]);
      else if (arg == "--warmups" && i + 1 < argc) config.warmups = std::stoi(argv[++i]);
      else if (arg == "--quadratic-limit" && i + 1 < argc) config.quadratic_limit = static_cast<std::size_t>(std::stoull(argv[++i]));
      else if (arg == "--sizes" && i + 1 < argc) config.sizes = parse_sizes(argv[++i]);
      else if (arg == "--payload-words" && i + 1 < argc) config.payload_words = parse_sizes(argv[++i]);
      else if (arg == "--algorithms" && i + 1 < argc) config.algorithm_names = split_csv(argv[++i]);
      else if (arg == "--patterns" && i + 1 < argc) config.pattern_names = split_csv(argv[++i]);
      else if (arg == "--help") {
        std::cout << "sort_records [--self-test] [--list-algorithms] [--list-payloads] [--environment]\n"
                     "             [--seed N] [--trials N] [--warmups N] [--quadratic-limit N]\n"
                     "             [--sizes a,b,c] [--payload-words 0,1,3,7,15,31]\n"
                     "             [--algorithms a,b] [--patterns a,b]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown or incomplete argument: " + arg);
      }
    }

    if (do_test) return self_test();
    if (list_algorithms) {
      std::cout << "algorithm,family,stable_guarantee,auxiliary\n";
      print_record_algorithms();
      return 0;
    }
    if (list_payloads) {
      print_payloads();
      return 0;
    }
    if (environment) {
      print_record_environment();
      return 0;
    }
    if (config.trials < 1) throw std::runtime_error("trials must be positive");
    if (config.warmups < 0) throw std::runtime_error("warmups must be nonnegative");
    if (config.sizes.empty()) throw std::runtime_error("sizes must not be empty");
    if (config.payload_words.empty()) throw std::runtime_error("payload widths must not be empty");

    const auto patterns = selected_record_patterns(config.pattern_names);
    std::cout << "schema_version,algorithm,pattern,n,payload_words,record_bytes,trial,experiment_seed,trial_seed,key_hash,input_hash,execution_order,ns,comparisons,swaps,explicit_record_moves,explicit_bytes_moved,stable_on_trial,verified\n";
    for (const auto words : config.payload_words) dispatch_width(words, config, patterns);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 2;
  }
}
