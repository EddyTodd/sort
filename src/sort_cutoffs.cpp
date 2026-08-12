#include "sortlab/extended_algorithms.hpp"
#include "sortlab/workloads.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace sortlab;

namespace {

struct Variant {
  std::string family;
  std::size_t cutoff;
};

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
  for (const auto& token : split(text)) {
    result.push_back(static_cast<std::size_t>(std::stoull(token)));
  }
  return result;
}

template <bool Count>
void run_variant(const Variant& variant, std::vector<Value>& values, Stats& stats) {
  if (variant.family == "merge_insertion") {
    merge_insertion_runtime<Count>(values, stats, variant.cutoff);
  } else if (variant.family == "quick_insertion") {
    quick_insertion_runtime<Count>(values, stats, variant.cutoff);
  } else if (variant.family == "intro_insertion") {
    intro_cutoff_runtime<Count>(values, stats, variant.cutoff);
  } else {
    throw std::runtime_error("unknown family " + variant.family);
  }
}

int self_test() {
  for (const std::string family : {"merge_insertion", "quick_insertion", "intro_insertion"}) {
    for (const std::size_t cutoff : {1U, 4U, 16U, 24U, 64U, 128U}) {
      for (const auto& pattern : all_patterns()) {
        for (const std::size_t n : {0U, 1U, 2U, 17U, 65U, 257U}) {
          const auto input = make_data(pattern, n, trial_seed(7, pattern, n, 3));
          auto copy = input;
          Stats stats;
          run_variant<false>({family, cutoff}, copy, stats);
          if (!verify(input, copy)) {
            std::cerr << "FAIL " << family << " cutoff=" << cutoff
                      << " pattern=" << pattern << " n=" << n << '\n';
            return 1;
          }
        }
      }
    }
  }
  std::cout << "PASS: cutoff families across deterministic workloads\n";
  return 0;
}

struct Measurement {
  std::size_t variant_index{};
  std::size_t execution_order{};
  std::int64_t ns{};
  Stats stats{};
};

}  // namespace

int main(int argc, char** argv) {
  try {
    bool test = false;
    int trials = 31;
    int warmups = 1;
    std::uint64_t seed = 0xC07FFULL;
    std::vector<std::size_t> sizes = {8, 12, 16, 24, 32, 48, 64, 96, 128,
                                      192, 256, 512, 1024, 2048, 4096, 8192};
    std::vector<std::size_t> cutoffs = {1, 4, 8, 12, 16, 20, 24, 32, 48, 64, 96, 128};
    std::vector<std::string> families = {"merge_insertion", "quick_insertion", "intro_insertion"};
    std::vector<std::string> patterns = {"random", "sorted", "few_unique", "nearly_sorted", "runs"};

    for (int i = 1; i < argc; ++i) {
      const std::string argument = argv[i];
      if (argument == "--self-test") test = true;
      else if (argument == "--trials" && i + 1 < argc) trials = std::stoi(argv[++i]);
      else if (argument == "--warmups" && i + 1 < argc) warmups = std::stoi(argv[++i]);
      else if (argument == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
      else if (argument == "--sizes" && i + 1 < argc) sizes = numbers(argv[++i]);
      else if (argument == "--cutoffs" && i + 1 < argc) cutoffs = numbers(argv[++i]);
      else if (argument == "--families" && i + 1 < argc) families = split(argv[++i]);
      else if (argument == "--patterns" && i + 1 < argc) patterns = split(argv[++i]);
      else if (argument == "--help") {
        std::cout << "sort_cutoffs [--trials N] [--warmups N] [--seed N] "
                     "[--sizes csv] [--cutoffs csv] [--families csv] [--patterns csv]\n";
        return 0;
      } else {
        throw std::runtime_error("bad argument: " + argument);
      }
    }

    if (test) return self_test();
    if (trials < 1 || warmups < 0) throw std::runtime_error("invalid trial/warmup count");

    std::vector<Variant> variants;
    for (const auto& family : families) {
      for (const std::size_t cutoff : cutoffs) variants.push_back({family, cutoff});
    }

    std::cout << "schema_version,family,cutoff,pattern,n,trial,experiment_seed,"
                 "trial_seed,input_hash,execution_order,ns,comparisons,swaps,writes,verified\n";

    for (const std::size_t n : sizes) {
      for (const auto& pattern : patterns) {
        for (int warmup = 0; warmup < warmups; ++warmup) {
          const auto seed_value = trial_seed(
              seed ^ 0xA0761D6478BD642FULL, pattern, n, static_cast<std::uint64_t>(warmup));
          const auto input = make_data(pattern, n, seed_value);
          for (const auto& variant : variants) {
            auto copy = input;
            Stats stats;
            run_variant<false>(variant, copy, stats);
            if (!verify(input, copy)) throw std::runtime_error("warmup verify failed");
          }
        }

        for (int trial = 0; trial < trials; ++trial) {
          const auto seed_value = trial_seed(seed, pattern, n, static_cast<std::uint64_t>(trial));
          const auto input = make_data(pattern, n, seed_value);
          const auto fingerprint = input_hash(input);
          std::vector<std::size_t> order(variants.size());
          std::iota(order.begin(), order.end(), 0);
          deterministic_shuffle(order, splitmix64(seed_value ^ 0xD1B54A32D192ED03ULL));

          std::vector<Measurement> measurements;
          measurements.reserve(order.size());
          std::size_t execution_order = 0;
          for (const std::size_t index : order) {
            auto copy = input;
            Stats stats;
            const auto start = std::chrono::steady_clock::now();
            run_variant<false>(variants[index], copy, stats);
            const auto stop = std::chrono::steady_clock::now();
            if (!verify(input, copy)) throw std::runtime_error("timed verify failed");
            measurements.push_back({
                index, execution_order++,
                std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count(), {}});
          }

          for (auto& measurement : measurements) {
            auto copy = input;
            run_variant<true>(variants[measurement.variant_index], copy, measurement.stats);
            if (!verify(input, copy)) throw std::runtime_error("instrumented verify failed");
          }

          for (const auto& measurement : measurements) {
            const auto& variant = variants[measurement.variant_index];
            std::cout << 1 << ',' << variant.family << ',' << variant.cutoff << ',' << pattern
                      << ',' << n << ',' << trial << ',' << seed << ',' << seed_value << ','
                      << fingerprint << ',' << measurement.execution_order << ',' << measurement.ns
                      << ',' << measurement.stats.comparisons << ',' << measurement.stats.swaps
                      << ',' << measurement.stats.writes << ",1\n";
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
