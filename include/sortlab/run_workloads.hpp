#pragma once

#include "sortlab/common.hpp"
#include "sortlab/workloads.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sortlab {

inline std::vector<std::size_t> normalized_run_lengths(std::size_t n,
                                                        const std::vector<std::size_t>& cycle) {
  std::vector<std::size_t> lengths;
  if (n == 0) return lengths;
  std::size_t remaining = n;
  std::size_t index = 0;
  while (remaining > 0) {
    std::size_t len = std::min(remaining, cycle[index % cycle.size()]);
    if (remaining > len && remaining - len == 1) ++len;
    if (len == 1 && !lengths.empty()) {
      ++lengths.back();
      break;
    }
    lengths.push_back(len);
    remaining -= len;
    ++index;
  }
  return lengths;
}

inline std::vector<std::size_t> run_shape_lengths(std::string_view pattern,
                                                   std::size_t n) {
  if (pattern == "run_equal32" || pattern == "run_alternating_direction") {
    return normalized_run_lengths(n, {32});
  }
  if (pattern == "run_long_short") {
    return normalized_run_lengths(n, {8, 128});
  }
  if (pattern == "run_power_skew") {
    return normalized_run_lengths(n, {8, 16, 32, 64, 128, 256, 32, 16});
  }
  if (pattern == "run_fibonacci") {
    return normalized_run_lengths(n, {13, 21, 34, 55, 89, 144, 233});
  }
  throw std::runtime_error("unknown run-shape pattern: " + std::string(pattern));
}

inline std::vector<Value> make_run_shaped_data(std::string_view pattern,
                                               std::size_t n) {
  const auto lengths = run_shape_lengths(pattern, n);
  std::vector<Value> values;
  values.reserve(n);

  if (pattern == "run_alternating_direction") {
    // Construct exact alternating monotone runs.  After a descending run, the
    // next ascending run starts one value above the previous endpoint so the
    // strict-descending detector cannot consume that first element.
    Value cursor = static_cast<Value>(n * 4 + 1024);
    for (std::size_t r = 0; r < lengths.size(); ++r) {
      const std::size_t len = lengths[r];
      if ((r % 2) == 0) {
        const Value low = cursor - static_cast<Value>(len) + 1;
        for (std::size_t i = 0; i < len; ++i) {
          values.push_back(low + static_cast<Value>(i));
        }
        cursor = low - 1;
      } else {
        const Value high = cursor;
        const Value low = high - static_cast<Value>(len) + 1;
        for (std::size_t i = 0; i < len; ++i) {
          values.push_back(high - static_cast<Value>(i));
        }
        cursor = low + static_cast<Value>(len);
      }
    }
    return values;
  }

  Value high = static_cast<Value>(n * 4 + 1024);
  for (const std::size_t len : lengths) {
    const Value low = high - static_cast<Value>(len);
    for (std::size_t i = 0; i < len; ++i) {
      values.push_back(low + static_cast<Value>(i));
    }
    high = low - 7;
  }
  return values;
}

inline std::vector<std::size_t> natural_run_lengths(const std::vector<Value>& values) {
  std::vector<std::size_t> lengths;
  std::size_t lo = 0;
  while (lo < values.size()) {
    if (lo + 1 == values.size()) {
      lengths.push_back(1);
      break;
    }
    std::size_t hi = lo + 2;
    if (values[lo + 1] < values[lo]) {
      while (hi < values.size() && values[hi] < values[hi - 1]) ++hi;
    } else {
      while (hi < values.size() && !(values[hi] < values[hi - 1])) ++hi;
    }
    lengths.push_back(hi - lo);
    lo = hi;
  }
  return lengths;
}

inline double run_length_entropy_bits(const std::vector<std::size_t>& lengths,
                                      std::size_t n) {
  if (n == 0) return 0.0;
  double entropy = 0.0;
  for (const auto len : lengths) {
    const double p = static_cast<double>(len) / static_cast<double>(n);
    entropy -= p * std::log2(p);
  }
  return entropy;
}

inline bool is_run_shape_pattern(std::string_view pattern) {
  return pattern == "run_equal32" || pattern == "run_long_short" ||
         pattern == "run_power_skew" || pattern == "run_fibonacci" ||
         pattern == "run_alternating_direction";
}

inline const std::vector<std::string>& merge_policy_patterns() {
  static const std::vector<std::string> patterns = {
      "random", "sorted", "reversed", "few_unique", "nearly_sorted", "runs",
      "descending_runs", "run_equal32", "run_long_short", "run_power_skew",
      "run_fibonacci", "run_alternating_direction"};
  return patterns;
}

inline std::vector<Value> make_merge_policy_data(std::string_view pattern,
                                                 std::size_t n,
                                                 std::uint64_t seed) {
  if (is_run_shape_pattern(pattern)) return make_run_shaped_data(pattern, n);
  return make_data(pattern, n, seed);
}

}  // namespace sortlab
