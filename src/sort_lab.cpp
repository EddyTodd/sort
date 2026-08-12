#include "sortlab/algorithms.hpp"
#include "sortlab/workloads.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

using namespace sortlab;

static std::vector<std::size_t> selected_algorithms(const std::vector<std::string>& names) {
  std::vector<std::size_t> indexes;
  const auto& table = algorithms();
  if (names.empty()) {
    indexes.resize(table.size());
    std::iota(indexes.begin(), indexes.end(), 0);
    return indexes;
  }
  for (const auto& name : names) {
    const auto it = std::find_if(table.begin(), table.end(), [&](const Algorithm& alg) { return alg.name == name; });
    if (it == table.end()) throw std::runtime_error("unknown algorithm: " + name);
    indexes.push_back(static_cast<std::size_t>(std::distance(table.begin(), it)));
  }
  return indexes;
}
static int self_test() {
  std::vector<std::vector<Value>> cases = {{}, {1}, {2, 1}, {1, 1, 1},
      {3, -1, 2, 2, 0}, {5, 4, 3, 2, 1}, {1, 2, 3, 4, 5},
      {std::numeric_limits<Value>::min(), 0, std::numeric_limits<Value>::max()}};
  for (const auto& pattern : all_patterns()) {
    for (const std::size_t n : {0U, 1U, 2U, 3U, 17U, 64U, 127U}) {
      cases.push_back(make_data(pattern, n, trial_seed(1234567, pattern, n, 9)));
    }
  }
  const auto& table = algorithms();
  for (const auto& alg : table) {
    for (const auto& input : cases) {
      auto timed = input;
      Stats unused;
      alg.timed(timed, unused);
      if (!verify(input, timed)) {
        std::cerr << "FAIL timed " << alg.name << " n=" << input.size() << '\n';
        return 1;
      }
      auto instrumented = input;
      Stats stats;
      alg.instrumented(instrumented, stats);
      if (!verify(input, instrumented) || instrumented != timed) {
        std::cerr << "FAIL instrumented " << alg.name << " n=" << input.size() << '\n';
        return 1;
      }
    }
  }
  const auto seed_a = trial_seed(99, "random", 1024, 7);
  const auto seed_b = trial_seed(99, "random", 1024, 8);
  const auto data_a = make_data("random", 1024, seed_a);
  const auto data_a2 = make_data("random", 1024, seed_a);
  const auto data_b = make_data("random", 1024, seed_b);
  if (data_a != data_a2 || data_a == data_b || input_hash(data_a) != input_hash(data_a2)) {
    std::cerr << "FAIL reproducibility contract\n";
    return 1;
  }
  std::cout << "PASS: " << table.size() << " algorithms, " << all_patterns().size()
            << " workload families, " << cases.size() << " deterministic cases\n";
  return 0;
}

static void print_algorithms() {
  std::cout << "algorithm,family,comparison_based,stable,in_place,adaptive,best,average,worst,auxiliary\n";
  for (const auto& alg : algorithms()) {
    std::cout << alg.name << ',' << alg.family << ',' << alg.comparison_based << ',' << alg.stable << ','
              << alg.in_place << ',' << alg.adaptive << ',' << alg.best << ',' << alg.average << ','
              << alg.worst << ',' << alg.auxiliary << '\n';
  }
}

static void print_environment() {
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
  std::cout << "{\"schema_version\":2,\"compiler\":\"" << compiler
            << "\",\"cplusplus\":" << __cplusplus
            << ",\"value_bits\":" << (sizeof(Value) * 8)
            << ",\"pointer_bits\":" << (sizeof(void*) * 8)
            << ",\"steady_clock\":" << (std::chrono::steady_clock::is_steady ? "true" : "false")
            << ",\"assertions_enabled\":" << assertions << "}\n";
}

struct Measurement {
  std::size_t algorithm_index{};
  std::size_t execution_order{};
  std::int64_t ns{};
  Stats stats{};
  bool verified{};
};

int main(int argc, char** argv) {
  try {
    bool do_test = false;
    bool list = false;
    bool environment = false;
    std::uint64_t seed = 0x5EEDULL;
    int trials = 11;
    int warmups = 1;
    std::size_t quadratic_limit = 16384;
    std::vector<std::size_t> sizes = {32, 256, 2048, 16384, 131072};
    std::vector<std::string> algorithm_names;
    std::vector<std::string> pattern_names;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--self-test") do_test = true;
      else if (arg == "--list-algorithms") list = true;
      else if (arg == "--environment") environment = true;
      else if (arg == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
      else if (arg == "--trials" && i + 1 < argc) trials = std::stoi(argv[++i]);
      else if (arg == "--warmups" && i + 1 < argc) warmups = std::stoi(argv[++i]);
      else if (arg == "--quadratic-limit" && i + 1 < argc) quadratic_limit = static_cast<std::size_t>(std::stoull(argv[++i]));
      else if (arg == "--sizes" && i + 1 < argc) sizes = parse_sizes(argv[++i]);
      else if (arg == "--algorithms" && i + 1 < argc) algorithm_names = split_csv(argv[++i]);
      else if (arg == "--patterns" && i + 1 < argc) pattern_names = split_csv(argv[++i]);
      else if (arg == "--help") {
        std::cout << "sort_lab [--self-test] [--list-algorithms] [--environment]\n"
                     "         [--seed N] [--trials N] [--warmups N] [--quadratic-limit N]\n"
                     "         [--sizes a,b,c] [--algorithms a,b] [--patterns a,b]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown or incomplete argument: " + arg);
      }
    }
    if (do_test) return self_test();
    if (list) {
      print_algorithms();
      return 0;
    }
    if (environment) {
      print_environment();
      return 0;
    }
    if (trials < 1) throw std::runtime_error("trials must be positive");
    if (warmups < 0) throw std::runtime_error("warmups must be nonnegative");
    if (sizes.empty()) throw std::runtime_error("sizes must not be empty");

    const auto selected = selected_algorithms(algorithm_names);
    const auto patterns = selected_patterns(pattern_names);
    const auto& table = algorithms();
    std::cout << "schema_version,algorithm,pattern,n,trial,experiment_seed,trial_seed,input_hash,execution_order,ns,comparisons,swaps,writes,verified\n";

    for (const auto n : sizes) {
      for (const auto& pattern : patterns) {
        for (int w = 0; w < warmups; ++w) {
          const auto wseed = trial_seed(seed ^ 0xA0761D6478BD642FULL, pattern, n, static_cast<std::uint64_t>(w));
          const auto input = make_data(pattern, n, wseed);
          for (const auto idx : selected) {
            const auto& alg = table[idx];
            if (alg.quadratic && n > quadratic_limit) continue;
            auto copy = input;
            Stats unused;
            alg.timed(copy, unused);
            if (!verify(input, copy)) throw std::runtime_error("warmup verification failed for " + std::string(alg.name));
          }
        }

        for (int trial = 0; trial < trials; ++trial) {
          const auto tseed = trial_seed(seed, pattern, n, static_cast<std::uint64_t>(trial));
          const auto input = make_data(pattern, n, tseed);
          const auto fingerprint = input_hash(input);
          auto order = selected;
          deterministic_shuffle(order, splitmix64(tseed ^ 0xE7037ED1A0B428DBULL));
          std::vector<Measurement> measurements;
          measurements.reserve(order.size());

          std::size_t executed = 0;
          for (const auto idx : order) {
            const auto& alg = table[idx];
            if (alg.quadratic && n > quadratic_limit) continue;
            auto copy = input;
            Stats unused;
            const auto start = std::chrono::steady_clock::now();
            alg.timed(copy, unused);
            const auto stop = std::chrono::steady_clock::now();
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            const bool ok = verify(input, copy);
            measurements.push_back({idx, executed++, ns, {}, ok});
            if (!ok) throw std::runtime_error("timed verification failed for " + std::string(alg.name));
          }

          for (auto& measurement : measurements) {
            const auto& alg = table[measurement.algorithm_index];
            auto copy = input;
            alg.instrumented(copy, measurement.stats);
            const bool ok = verify(input, copy);
            measurement.verified = measurement.verified && ok;
            if (!ok) throw std::runtime_error("instrumented verification failed for " + std::string(alg.name));
          }

          for (const auto& measurement : measurements) {
            const auto& alg = table[measurement.algorithm_index];
            std::cout << 2 << ',' << alg.name << ',' << pattern << ',' << n << ',' << trial << ','
                      << seed << ',' << tseed << ',' << fingerprint << ',' << measurement.execution_order << ','
                      << measurement.ns << ',' << measurement.stats.comparisons << ',' << measurement.stats.swaps << ','
                      << measurement.stats.writes << ',' << (measurement.verified ? 1 : 0) << '\n';
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
