#include "sortlab/allocation_tracker.hpp"
#include "sortlab/extended_algorithms.hpp"
#include "sortlab/workloads.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

allocation_tracker::Snapshot measure(const Algorithm& algorithm,
                                     const std::vector<Value>& input) {
  auto copy = input;
  Stats stats;
  allocation_tracker::start();
  algorithm.timed(copy, stats);
  const auto snapshot = allocation_tracker::stop();
  if (!verify(input, copy)) {
    throw std::runtime_error("verification failed for " + std::string(algorithm.name));
  }
  return snapshot;
}

int self_test() {
  const auto input = make_data("random", 4096, trial_seed(1, "random", 4096, 0));
  const auto& table = all_algorithms();
  const auto find = [&](std::string_view name) -> const Algorithm& {
    return *std::find_if(table.begin(), table.end(),
                         [&](const Algorithm& algorithm) { return algorithm.name == name; });
  };
  const auto heap = measure(find("heap"), input);
  const auto merge = measure(find("merge"), input);
  const auto radix = measure(find("radix_lsd"), input);
  if (heap.calls != 0 || merge.calls == 0 || radix.calls == 0) {
    std::cerr << "FAIL allocation classification\n";
    return 1;
  }
  std::cout << "PASS: allocation tracker distinguishes in-place and allocating algorithms\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    bool test = false;
    int trials = 5;
    std::uint64_t seed = 0xA110CULL;
    std::vector<std::size_t> sizes = {1024, 16384, 262144};
    std::vector<std::string> patterns = {"random", "few_unique", "nearly_sorted"};
    std::vector<std::string> names;

    for (int i = 1; i < argc; ++i) {
      const std::string argument = argv[i];
      if (argument == "--self-test") test = true;
      else if (argument == "--trials" && i + 1 < argc) trials = std::stoi(argv[++i]);
      else if (argument == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
      else if (argument == "--sizes" && i + 1 < argc) sizes = numbers(argv[++i]);
      else if (argument == "--patterns" && i + 1 < argc) patterns = split(argv[++i]);
      else if (argument == "--algorithms" && i + 1 < argc) names = split(argv[++i]);
      else if (argument == "--help") {
        std::cout << "sort_alloc [--trials N] [--sizes csv] [--patterns csv] "
                     "[--algorithms csv]\n";
        return 0;
      } else {
        throw std::runtime_error("bad argument: " + argument);
      }
    }

    if (test) return self_test();
    if (trials < 1) throw std::runtime_error("trials must be positive");

    const auto selected = select_algorithms(names);
    const auto& table = all_algorithms();
    std::cout << "schema_version,algorithm,pattern,n,trial,trial_seed,input_hash,"
                 "allocation_calls,total_requested_bytes,peak_live_bytes,"
                 "max_single_allocation_bytes,live_bytes_at_stop,verified\n";

    for (const std::size_t n : sizes) {
      for (const auto& pattern : patterns) {
        for (int trial = 0; trial < trials; ++trial) {
          const auto seed_value = trial_seed(seed, pattern, n, static_cast<std::uint64_t>(trial));
          const auto input = make_data(pattern, n, seed_value);
          const auto fingerprint = input_hash(input);
          for (const std::size_t index : selected) {
            const auto snapshot = measure(table[index], input);
            std::cout << 1 << ',' << table[index].name << ',' << pattern << ',' << n << ','
                      << trial << ',' << seed_value << ',' << fingerprint << ',' << snapshot.calls
                      << ',' << snapshot.total_requested << ',' << snapshot.peak_live << ','
                      << snapshot.max_single << ',' << snapshot.live_at_stop << ",1\n";
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
