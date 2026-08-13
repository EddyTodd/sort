#pragma once

#include "sortlab/record_algorithms.hpp"
#include "sortlab/workloads.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sortlab {

static std::vector<Value> make_record_keys(std::string_view pattern, std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::vector<Value> keys(n);
  for (auto& key : keys) key = static_cast<Value>(bounded(rng, 2000000001ULL)) - 1000000000LL;

  if (pattern == "random") return keys;
  if (pattern == "sorted") {
    std::sort(keys.begin(), keys.end());
    return keys;
  }
  if (pattern == "reversed") {
    std::sort(keys.begin(), keys.end(), std::greater<>());
    return keys;
  }
  if (pattern == "few_unique") {
    for (auto& key : keys) key = static_cast<Value>(bounded(rng, 8));
    return keys;
  }
  if (pattern == "binary") {
    for (auto& key : keys) key = static_cast<Value>(bounded(rng, 2));
    return keys;
  }
  if (pattern == "all_equal") {
    std::fill(keys.begin(), keys.end(), 7);
    return keys;
  }
  if (pattern == "nearly_sorted") {
    std::sort(keys.begin(), keys.end());
    if (n > 1) {
      const std::size_t swaps = std::max<std::size_t>(1, n / 100);
      for (std::size_t k = 0; k < swaps; ++k) {
        const auto i = static_cast<std::size_t>(bounded(rng, static_cast<std::uint64_t>(n)));
        const auto j = static_cast<std::size_t>(bounded(rng, static_cast<std::uint64_t>(n)));
        std::swap(keys[i], keys[j]);
      }
    }
    return keys;
  }
  if (pattern == "organ_pipe") {
    for (std::size_t i = 0; i < n; ++i) keys[i] = static_cast<Value>(std::min(i, n == 0 ? 0 : n - 1 - i));
    return keys;
  }
  if (pattern == "sawtooth") {
    for (std::size_t i = 0; i < n; ++i) keys[i] = static_cast<Value>(i % 32);
    return keys;
  }
  if (pattern == "runs" || pattern == "descending_runs") {
    constexpr std::size_t run = 32;
    for (std::size_t base = 0; base < n; base += run) {
      const auto end = std::min(n, base + run);
      if (pattern == "runs") {
        std::sort(keys.begin() + static_cast<std::ptrdiff_t>(base),
                  keys.begin() + static_cast<std::ptrdiff_t>(end));
      } else {
        std::sort(keys.begin() + static_cast<std::ptrdiff_t>(base),
                  keys.begin() + static_cast<std::ptrdiff_t>(end), std::greater<>());
      }
    }
    return keys;
  }
  if (pattern == "rotated") {
    std::sort(keys.begin(), keys.end());
    if (n > 1) std::rotate(keys.begin(), keys.begin() + static_cast<std::ptrdiff_t>(n / 3), keys.end());
    return keys;
  }
  if (pattern == "alternating_extremes") {
    Value low = std::numeric_limits<Value>::min();
    Value high = std::numeric_limits<Value>::max();
    for (std::size_t i = 0; i < n; ++i) keys[i] = (i % 2 == 0) ? low++ : high--;
    return keys;
  }
  if (pattern == "staggered") {
    constexpr std::size_t modulus = 97;
    for (std::size_t i = 0; i < n; ++i) keys[i] = static_cast<Value>((i * 37) % modulus);
    return keys;
  }
  if (pattern == "plateau") {
    for (std::size_t i = 0; i < n; ++i) {
      const auto distance = std::min(i, n == 0 ? 0 : n - 1 - i);
      keys[i] = static_cast<Value>(std::min<std::size_t>(distance, 16));
    }
    return keys;
  }
  throw std::runtime_error("unknown record pattern: " + std::string(pattern));
}

static const std::vector<std::string>& all_record_patterns() {
  static const std::vector<std::string> patterns = {
      "random", "sorted", "reversed", "few_unique", "binary", "all_equal",
      "nearly_sorted", "organ_pipe", "sawtooth", "runs", "descending_runs",
      "rotated", "alternating_extremes", "staggered", "plateau"};
  return patterns;
}

static std::vector<std::string> selected_record_patterns(const std::vector<std::string>& names) {
  if (names.empty()) return all_record_patterns();
  for (const auto& name : names) {
    if (std::find(all_record_patterns().begin(), all_record_patterns().end(), name) == all_record_patterns().end()) {
      throw std::runtime_error("unknown record pattern: " + name);
    }
  }
  return names;
}

static std::uint64_t key_hash(const std::vector<Value>& keys) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (const auto key : keys) {
    std::uint64_t bits = static_cast<std::uint64_t>(key);
    for (int byte = 0; byte < 8; ++byte) {
      hash ^= bits & 0xffU;
      hash *= 1099511628211ULL;
      bits >>= 8;
    }
  }
  return hash;
}

template <std::size_t Words>
static std::vector<Record<Words>> make_records(std::string_view pattern, std::size_t n, std::uint64_t seed) {
  const auto keys = make_record_keys(pattern, n, seed);
  std::vector<Record<Words>> records(n);
  for (std::size_t i = 0; i < n; ++i) {
    records[i].key = keys[i];
    records[i].ordinal = static_cast<std::uint64_t>(i);
    if constexpr (Words > 0) {
      for (std::size_t word = 0; word < Words; ++word) {
        records[i].payload.words[word] = splitmix64(seed ^ (static_cast<std::uint64_t>(i) * 0xD6E8FEB86659FD93ULL) ^
                                                    static_cast<std::uint64_t>(word));
      }
    }
  }
  return records;
}

template <std::size_t Words>
static std::uint64_t record_hash(const std::vector<Record<Words>>& records) {
  std::uint64_t hash = 14695981039346656037ULL;
  const auto absorb = [&](std::uint64_t value, std::uint64_t& state) {
    for (int byte = 0; byte < 8; ++byte) {
      state ^= value & 0xffU;
      state *= 1099511628211ULL;
      value >>= 8;
    }
  };
  for (const auto& record : records) {
    absorb(static_cast<std::uint64_t>(record.key), hash);
    absorb(record.ordinal, hash);
    if constexpr (Words > 0) {
      for (const auto word : record.payload.words) absorb(word, hash);
    }
  }
  return hash;
}

struct RecordVerification {
  bool correct = false;
  bool stable = false;
};

template <std::size_t Words>
static RecordVerification verify_records(const std::vector<Record<Words>>& input,
                                         const std::vector<Record<Words>>& output) {
  if (input.size() != output.size()) return {};
  std::vector<bool> seen(input.size(), false);
  bool stable = true;
  for (std::size_t i = 0; i < output.size(); ++i) {
    const auto& record = output[i];
    if (record.ordinal >= input.size()) return {};
    const auto index = static_cast<std::size_t>(record.ordinal);
    if (seen[index] || !(record == input[index])) return {};
    seen[index] = true;
    if (i > 0) {
      if (output[i - 1].key > record.key) return {};
      if (output[i - 1].key == record.key && output[i - 1].ordinal > record.ordinal) stable = false;
    }
  }
  return {true, stable};
}

}  // namespace sortlab
