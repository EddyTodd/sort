#pragma once

#include "sortlab/common.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace sortlab {

enum class MergePolicy { pairwise, timsort_stack, powersort };
enum class MinrunPolicy { none, classic, balanced };

inline const char* merge_policy_name(MergePolicy policy) {
  switch (policy) {
    case MergePolicy::pairwise: return "pairwise";
    case MergePolicy::timsort_stack: return "timsort_stack";
    case MergePolicy::powersort: return "powersort";
  }
  return "unknown";
}

inline const char* minrun_policy_name(MinrunPolicy policy) {
  switch (policy) {
    case MinrunPolicy::none: return "none";
    case MinrunPolicy::classic: return "classic";
    case MinrunPolicy::balanced: return "balanced";
  }
  return "unknown";
}

struct AdaptiveMergeMetrics {
  std::uint64_t natural_runs = 0;
  std::uint64_t effective_runs = 0;
  std::uint64_t reversed_runs = 0;
  std::uint64_t extended_elements = 0;
  std::uint64_t merges = 0;
  std::uint64_t scheduled_merge_cost = 0;
  std::uint64_t max_pending_runs = 0;
  double run_entropy_bits = 0.0;
};

struct AdaptiveRun {
  std::size_t base = 0;
  std::size_t len = 0;
  int power = 0;
};

inline std::size_t classic_minrun(std::size_t n) {
  std::size_t r = 0;
  while (n >= 64) {
    r |= n & 1U;
    n >>= 1U;
  }
  return n + r;
}

struct BalancedMinrunState {
  std::size_t n = 0;
  unsigned e = 0;
  std::size_t mask = 0;
  std::size_t current = 0;

  explicit BalancedMinrunState(std::size_t size) : n(size) {
    while ((n >> e) >= 64) ++e;
    mask = e == 0 ? 0 : (std::size_t{1} << e) - 1;
  }

  std::size_t next() {
    current += n;
    const std::size_t result = current >> e;
    current &= mask;
    return result;
  }
};

template <bool Count>
inline std::size_t count_run_and_make_ascending(std::vector<Value>& values,
                                                std::size_t lo,
                                                AdaptiveMergeMetrics& metrics,
                                                Stats& stats) {
  const std::size_t n = values.size();
  if (lo >= n) return 0;
  if (lo + 1 >= n) return 1;

  std::size_t hi = lo + 2;
  if (lessv<Count>(values[lo + 1], values[lo], stats)) {
    while (hi < n && lessv<Count>(values[hi], values[hi - 1], stats)) ++hi;
    for (std::size_t left = lo, right = hi - 1; left < right; ++left, --right) {
      swapv<Count>(values[left], values[right], stats);
    }
    if constexpr (Count) ++metrics.reversed_runs;
  } else {
    while (hi < n && !lessv<Count>(values[hi], values[hi - 1], stats)) ++hi;
  }
  return hi - lo;
}

template <bool Count>
inline void binary_extend_sorted_prefix(std::vector<Value>& values,
                                        std::size_t lo,
                                        std::size_t sorted_hi,
                                        std::size_t hi,
                                        Stats& stats) {
  for (std::size_t i = sorted_hi; i < hi; ++i) {
    const Value value = values[i];
    std::size_t left = lo;
    std::size_t right = i;
    while (left < right) {
      const std::size_t mid = left + (right - left) / 2;
      if (lessv<Count>(value, values[mid], stats)) right = mid;
      else left = mid + 1;
    }
    for (std::size_t j = i; j > left; --j) {
      writev<Count>(values[j], values[j - 1], stats);
    }
    writev<Count>(values[left], value, stats);
  }
}

template <bool Count>
inline void stable_merge_adjacent(std::vector<Value>& values,
                                  std::vector<Value>& temp,
                                  const AdaptiveRun& left,
                                  const AdaptiveRun& right,
                                  Stats& stats) {
  if (left.base + left.len != right.base) {
    throw std::runtime_error("adaptive merge received non-adjacent runs");
  }
  const std::size_t mid = right.base;
  const std::size_t hi = right.base + right.len;
  std::size_t i = left.base;
  std::size_t j = right.base;
  std::size_t k = left.base;
  while (i < mid && j < hi) {
    if (lessv<Count>(values[j], values[i], stats)) writev<Count>(temp[k++], values[j++], stats);
    else writev<Count>(temp[k++], values[i++], stats);
  }
  while (i < mid) writev<Count>(temp[k++], values[i++], stats);
  while (j < hi) writev<Count>(temp[k++], values[j++], stats);
  for (k = left.base; k < hi; ++k) writev<Count>(values[k], temp[k], stats);
}

template <bool Count>
inline void merge_stack_at(std::vector<Value>& values,
                           std::vector<Value>& temp,
                           std::vector<AdaptiveRun>& stack,
                           std::size_t index,
                           AdaptiveMergeMetrics& metrics,
                           Stats& stats) {
  if (index + 1 >= stack.size()) throw std::runtime_error("invalid merge stack index");
  const AdaptiveRun left = stack[index];
  const AdaptiveRun right = stack[index + 1];
  if constexpr (Count) {
    metrics.scheduled_merge_cost += static_cast<std::uint64_t>(left.len + right.len);
    ++metrics.merges;
  }
  stable_merge_adjacent<Count>(values, temp, left, right, stats);
  stack[index].base = left.base;
  stack[index].len = left.len + right.len;
  stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(index + 1));
}

inline int powerloop(std::size_t s1, std::size_t n1, std::size_t n2,
                     std::size_t n) {
  if (n == 0 || n1 == 0 || n2 == 0 || s1 + n1 + n2 > n) {
    throw std::runtime_error("invalid powersort run geometry");
  }
  if (n > std::numeric_limits<std::size_t>::max() / 2) {
    throw std::runtime_error("powersort input too large for exact integer powerloop");
  }
  int result = 0;
  std::size_t a = 2 * s1 + n1;
  std::size_t b = a + n1 + n2;
  for (;;) {
    ++result;
    if (a >= n) {
      a -= n;
      b -= n;
    } else if (b >= n) {
      break;
    }
    a <<= 1U;
    b <<= 1U;
  }
  return result;
}

template <bool Count>
inline void execute_pairwise_policy(std::vector<Value>& values,
                                    std::vector<Value>& temp,
                                    std::vector<AdaptiveRun> runs,
                                    AdaptiveMergeMetrics& metrics,
                                    Stats& stats) {
  if constexpr (Count) metrics.max_pending_runs = std::max<std::uint64_t>(metrics.max_pending_runs, runs.size());
  while (runs.size() > 1) {
    std::vector<AdaptiveRun> next;
    next.reserve((runs.size() + 1) / 2);
    for (std::size_t i = 0; i < runs.size(); i += 2) {
      if (i + 1 == runs.size()) {
        next.push_back(runs[i]);
        continue;
      }
      const AdaptiveRun left = runs[i];
      const AdaptiveRun right = runs[i + 1];
      if constexpr (Count) {
        metrics.scheduled_merge_cost += static_cast<std::uint64_t>(left.len + right.len);
        ++metrics.merges;
      }
      stable_merge_adjacent<Count>(values, temp, left, right, stats);
      next.push_back({left.base, left.len + right.len, 0});
    }
    runs.swap(next);
  }
}

template <bool Count>
inline void execute_timsort_policy(std::vector<Value>& values,
                                   std::vector<Value>& temp,
                                   const std::vector<AdaptiveRun>& runs,
                                   AdaptiveMergeMetrics& metrics,
                                   Stats& stats) {
  std::vector<AdaptiveRun> stack;
  for (const AdaptiveRun& run : runs) {
    stack.push_back(run);
    if constexpr (Count) metrics.max_pending_runs = std::max<std::uint64_t>(metrics.max_pending_runs, stack.size());
    while (stack.size() > 1) {
      std::size_t n = stack.size() - 2;
      const bool first = n > 0 && stack[n - 1].len <= stack[n].len + stack[n + 1].len;
      const bool second = n > 1 && stack[n - 2].len <= stack[n - 1].len + stack[n].len;
      if (first || second) {
        if (stack[n - 1].len < stack[n + 1].len) --n;
      } else if (stack[n].len > stack[n + 1].len) {
        break;
      }
      merge_stack_at<Count>(values, temp, stack, n, metrics, stats);
    }
  }
  while (stack.size() > 1) {
    std::size_t n = stack.size() - 2;
    if (n > 0 && stack[n - 1].len < stack[n + 1].len) --n;
    merge_stack_at<Count>(values, temp, stack, n, metrics, stats);
  }
}

template <bool Count>
inline void execute_powersort_policy(std::vector<Value>& values,
                                     std::vector<Value>& temp,
                                     const std::vector<AdaptiveRun>& runs,
                                     AdaptiveMergeMetrics& metrics,
                                     Stats& stats) {
  std::vector<AdaptiveRun> stack;
  const std::size_t total = values.size();
  for (const AdaptiveRun& incoming : runs) {
    if (!stack.empty()) {
      const AdaptiveRun top = stack.back();
      const int power = powerloop(top.base, top.len, incoming.len, total);
      while (stack.size() > 1 && stack[stack.size() - 2].power > power) {
        merge_stack_at<Count>(values, temp, stack, stack.size() - 2, metrics, stats);
      }
      stack.back().power = power;
    }
    stack.push_back(incoming);
    if constexpr (Count) metrics.max_pending_runs = std::max<std::uint64_t>(metrics.max_pending_runs, stack.size());
  }
  while (stack.size() > 1) {
    std::size_t n = stack.size() - 2;
    if (n > 0 && stack[n - 1].len < stack[n + 1].len) --n;
    merge_stack_at<Count>(values, temp, stack, n, metrics, stats);
  }
}

template <bool Count>
inline void adaptive_merge_sort(std::vector<Value>& values, Stats& stats,
                                MergePolicy merge_policy,
                                MinrunPolicy minrun_policy,
                                AdaptiveMergeMetrics& metrics) {
  if constexpr (Count) metrics = {};
  if (values.size() < 2) {
    if constexpr (Count) {
      if (!values.empty()) {
        metrics.natural_runs = 1;
        metrics.effective_runs = 1;
      }
    }
    return;
  }

  const std::size_t fixed_minrun = classic_minrun(values.size());
  BalancedMinrunState balanced(values.size());
  std::vector<AdaptiveRun> runs;
  for (std::size_t lo = 0; lo < values.size();) {
    const std::size_t natural = count_run_and_make_ascending<Count>(values, lo, metrics, stats);
    if constexpr (Count) ++metrics.natural_runs;
    std::size_t desired = natural;
    if (minrun_policy == MinrunPolicy::classic) desired = fixed_minrun;
    else if (minrun_policy == MinrunPolicy::balanced) desired = balanced.next();
    const std::size_t remaining = values.size() - lo;
    const std::size_t effective = std::min(remaining, std::max(natural, desired));
    if (effective > natural) {
      binary_extend_sorted_prefix<Count>(values, lo, lo + natural, lo + effective, stats);
      if constexpr (Count) metrics.extended_elements += static_cast<std::uint64_t>(effective - natural);
    }
    runs.push_back({lo, effective, 0});
    if constexpr (Count) ++metrics.effective_runs;
    lo += effective;
  }

  if constexpr (Count) {
    const double n = static_cast<double>(values.size());
    for (const auto& run : runs) {
      const double p = static_cast<double>(run.len) / n;
      metrics.run_entropy_bits -= p * std::log2(p);
    }
  }

  std::vector<Value> temp(values.size());
  switch (merge_policy) {
    case MergePolicy::pairwise:
      execute_pairwise_policy<Count>(values, temp, runs, metrics, stats);
      break;
    case MergePolicy::timsort_stack:
      execute_timsort_policy<Count>(values, temp, runs, metrics, stats);
      break;
    case MergePolicy::powersort:
      execute_powersort_policy<Count>(values, temp, runs, metrics, stats);
      break;
  }
}

}  // namespace sortlab
