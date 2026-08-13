#pragma once

#include "sortlab/adaptive_merge_kernels.hpp"
#include "sortlab/record_algorithms.hpp"
#include "sortlab/record_workloads.hpp"
#include "sortlab/run_workloads.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sortlab {

struct RecordMergeKernelMetrics {
  std::uint64_t merge_calls = 0;
  std::uint64_t gallop_entries = 0;
  std::uint64_t gallop_records = 0;
  std::uint64_t temp_records_peak = 0;
  std::uint64_t temp_capacity_peak = 0;
  std::uint64_t temp_records_copied = 0;
};

template <std::size_t Words>
struct RecordMergeWorkspace {
  std::vector<Record<Words>> storage;
};

template <bool Count>
inline bool record_key_less(Value left, Value right, RecordStats& stats) {
  if constexpr (Count) ++stats.comparisons;
  return left < right;
}

template <bool Count, std::size_t Words>
inline void ensure_record_workspace(RecordMergeWorkspace<Words>& workspace,
                                    std::size_t required,
                                    RecordMergeKernelMetrics& metrics) {
  if (workspace.storage.size() < required) workspace.storage.resize(required);
  if constexpr (Count) {
    metrics.temp_records_peak =
        std::max<std::uint64_t>(metrics.temp_records_peak, required);
    metrics.temp_capacity_peak = std::max<std::uint64_t>(
        metrics.temp_capacity_peak, workspace.storage.capacity());
  }
}

template <bool Count, class Accessor>
inline std::size_t record_lower_bound(std::size_t lo, std::size_t hi, Value key,
                                      Accessor&& at, RecordStats& stats) {
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2;
    if (record_key_less<Count>(at(mid).key, key, stats)) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return lo;
}

template <bool Count, class Accessor>
inline std::size_t record_upper_bound(std::size_t lo, std::size_t hi, Value key,
                                      Accessor&& at, RecordStats& stats) {
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2;
    if (!record_key_less<Count>(key, at(mid).key, stats)) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return lo;
}

template <bool Count, class Accessor>
inline std::size_t record_gallop_upper_forward(std::size_t lo, std::size_t hi,
                                               Value key, Accessor&& at,
                                               RecordStats& stats) {
  if (lo >= hi) return lo;
  if (record_key_less<Count>(key, at(lo).key, stats)) return lo;

  std::size_t last = lo;
  std::size_t offset = 1;
  while (offset < hi - lo) {
    const std::size_t probe = lo + offset;
    if (record_key_less<Count>(key, at(probe).key, stats)) {
      return record_upper_bound<Count>(last + 1, probe + 1, key,
                                       std::forward<Accessor>(at), stats);
    }
    last = probe;
    if (offset > (hi - lo) / 2) break;
    offset *= 2;
  }
  return record_upper_bound<Count>(last + 1, hi, key,
                                   std::forward<Accessor>(at), stats);
}

template <bool Count, class Accessor>
inline std::size_t record_gallop_lower_forward(std::size_t lo, std::size_t hi,
                                               Value key, Accessor&& at,
                                               RecordStats& stats) {
  if (lo >= hi) return lo;
  if (!record_key_less<Count>(at(lo).key, key, stats)) return lo;

  std::size_t last = lo;
  std::size_t offset = 1;
  while (offset < hi - lo) {
    const std::size_t probe = lo + offset;
    if (!record_key_less<Count>(at(probe).key, key, stats)) {
      return record_lower_bound<Count>(last + 1, probe + 1, key,
                                       std::forward<Accessor>(at), stats);
    }
    last = probe;
    if (offset > (hi - lo) / 2) break;
    offset *= 2;
  }
  return record_lower_bound<Count>(last + 1, hi, key,
                                   std::forward<Accessor>(at), stats);
}

template <bool Count, class Accessor>
inline std::size_t record_gallop_upper_reverse(std::size_t lo, std::size_t hi,
                                               Value key, Accessor&& at,
                                               RecordStats& stats) {
  if (lo >= hi) return hi;
  if (!record_key_less<Count>(key, at(hi - 1).key, stats)) return hi;

  std::size_t search_lo = lo;
  std::size_t search_hi = hi;
  std::size_t distance = 1;
  const std::size_t length = hi - lo;
  while (distance < length) {
    const std::size_t probe = hi - 1 - distance;
    if (record_key_less<Count>(key, at(probe).key, stats)) {
      search_hi = probe + 1;
      if (distance > length / 2) break;
      distance *= 2;
    } else {
      search_lo = probe + 1;
      break;
    }
  }
  return record_upper_bound<Count>(search_lo, search_hi, key,
                                   std::forward<Accessor>(at), stats);
}

template <bool Count, class Accessor>
inline std::size_t record_gallop_lower_reverse(std::size_t lo, std::size_t hi,
                                               Value key, Accessor&& at,
                                               RecordStats& stats) {
  if (lo >= hi) return hi;
  if (record_key_less<Count>(at(hi - 1).key, key, stats)) return hi;

  std::size_t search_lo = lo;
  std::size_t search_hi = hi;
  std::size_t distance = 1;
  const std::size_t length = hi - lo;
  while (distance < length) {
    const std::size_t probe = hi - 1 - distance;
    if (!record_key_less<Count>(at(probe).key, key, stats)) {
      search_hi = probe + 1;
      if (distance > length / 2) break;
      distance *= 2;
    } else {
      search_lo = probe + 1;
      break;
    }
  }
  return record_lower_bound<Count>(search_lo, search_hi, key,
                                   std::forward<Accessor>(at), stats);
}

template <bool Count>
inline void note_record_gallop(RecordMergeKernelMetrics& metrics,
                               std::size_t records) {
  if constexpr (Count) {
    ++metrics.gallop_entries;
    metrics.gallop_records += static_cast<std::uint64_t>(records);
  }
}

template <bool Count, std::size_t Words>
inline void record_merge_full_buffer(
    std::vector<Record<Words>>& values, RecordMergeWorkspace<Words>& workspace,
    const AdaptiveRun& left, const AdaptiveRun& right,
    const MergeKernelSpec& spec, RecordMergeKernelMetrics& metrics,
    RecordStats& stats) {
  const std::size_t total = left.len + right.len;
  ensure_record_workspace<Count>(workspace, total, metrics);
  auto& temp = workspace.storage;
  for (std::size_t x = 0; x < left.len; ++x) {
    record_write<Count>(temp[x], values[left.base + x], stats);
  }
  for (std::size_t x = 0; x < right.len; ++x) {
    record_write<Count>(temp[left.len + x], values[right.base + x], stats);
  }
  if constexpr (Count) {
    metrics.temp_records_copied += static_cast<std::uint64_t>(total);
  }

  std::size_t i = 0;
  std::size_t j = left.len;
  const std::size_t left_end = left.len;
  const std::size_t right_end = total;
  std::size_t out = left.base;
  std::size_t left_streak = 0;
  std::size_t right_streak = 0;
  const bool gallop = spec.search == MergeSearchPolicy::gallop;

  while (i < left_end && j < right_end) {
    if (record_less<Count>(temp[j], temp[i], stats)) {
      record_write<Count>(values[out++], temp[j++], stats);
      ++right_streak;
      left_streak = 0;
    } else {
      record_write<Count>(values[out++], temp[i++], stats);
      ++left_streak;
      right_streak = 0;
    }

    if (gallop && i < left_end && j < right_end &&
        left_streak >= spec.gallop_threshold) {
      const std::size_t end = record_gallop_upper_forward<Count>(
          i, left_end, temp[j].key,
          [&](std::size_t x) -> const Record<Words>& { return temp[x]; }, stats);
      const std::size_t count = end - i;
      note_record_gallop<Count>(metrics, count);
      while (i < end) record_write<Count>(values[out++], temp[i++], stats);
      left_streak = right_streak = 0;
    } else if (gallop && i < left_end && j < right_end &&
               right_streak >= spec.gallop_threshold) {
      const std::size_t end = record_gallop_lower_forward<Count>(
          j, right_end, temp[i].key,
          [&](std::size_t x) -> const Record<Words>& { return temp[x]; }, stats);
      const std::size_t count = end - j;
      note_record_gallop<Count>(metrics, count);
      while (j < end) record_write<Count>(values[out++], temp[j++], stats);
      left_streak = right_streak = 0;
    }
  }
  while (i < left_end) record_write<Count>(values[out++], temp[i++], stats);
  while (j < right_end) record_write<Count>(values[out++], temp[j++], stats);
}

template <bool Count, std::size_t Words>
inline void record_merge_smaller_left(
    std::vector<Record<Words>>& values, RecordMergeWorkspace<Words>& workspace,
    const AdaptiveRun& left, const AdaptiveRun& right,
    const MergeKernelSpec& spec, RecordMergeKernelMetrics& metrics,
    RecordStats& stats) {
  ensure_record_workspace<Count>(workspace, left.len, metrics);
  auto& temp = workspace.storage;
  for (std::size_t x = 0; x < left.len; ++x) {
    record_write<Count>(temp[x], values[left.base + x], stats);
  }
  if constexpr (Count) {
    metrics.temp_records_copied += static_cast<std::uint64_t>(left.len);
  }

  std::size_t i = 0;
  std::size_t j = right.base;
  std::size_t out = left.base;
  const std::size_t hi = right.base + right.len;
  std::size_t left_streak = 0;
  std::size_t right_streak = 0;
  const bool gallop = spec.search == MergeSearchPolicy::gallop;

  while (i < left.len && j < hi) {
    if (record_less<Count>(values[j], temp[i], stats)) {
      record_write<Count>(values[out++], values[j++], stats);
      ++right_streak;
      left_streak = 0;
    } else {
      record_write<Count>(values[out++], temp[i++], stats);
      ++left_streak;
      right_streak = 0;
    }

    if (gallop && i < left.len && j < hi &&
        left_streak >= spec.gallop_threshold) {
      const std::size_t end = record_gallop_upper_forward<Count>(
          i, left.len, values[j].key,
          [&](std::size_t x) -> const Record<Words>& { return temp[x]; }, stats);
      const std::size_t count = end - i;
      note_record_gallop<Count>(metrics, count);
      while (i < end) record_write<Count>(values[out++], temp[i++], stats);
      left_streak = right_streak = 0;
    } else if (gallop && i < left.len && j < hi &&
               right_streak >= spec.gallop_threshold) {
      const std::size_t end = record_gallop_lower_forward<Count>(
          j, hi, temp[i].key,
          [&](std::size_t x) -> const Record<Words>& { return values[x]; }, stats);
      const std::size_t count = end - j;
      note_record_gallop<Count>(metrics, count);
      while (j < end) record_write<Count>(values[out++], values[j++], stats);
      left_streak = right_streak = 0;
    }
  }
  while (i < left.len) record_write<Count>(values[out++], temp[i++], stats);
}

template <bool Count, std::size_t Words>
inline void record_merge_smaller_right(
    std::vector<Record<Words>>& values, RecordMergeWorkspace<Words>& workspace,
    const AdaptiveRun& left, const AdaptiveRun& right,
    const MergeKernelSpec& spec, RecordMergeKernelMetrics& metrics,
    RecordStats& stats) {
  ensure_record_workspace<Count>(workspace, right.len, metrics);
  auto& temp = workspace.storage;
  for (std::size_t x = 0; x < right.len; ++x) {
    record_write<Count>(temp[x], values[right.base + x], stats);
  }
  if constexpr (Count) {
    metrics.temp_records_copied += static_cast<std::uint64_t>(right.len);
  }

  std::size_t i = left.base + left.len;
  std::size_t j = right.len;
  std::size_t out = right.base + right.len;
  std::size_t left_streak = 0;
  std::size_t right_streak = 0;
  const bool gallop = spec.search == MergeSearchPolicy::gallop;

  while (i > left.base && j > 0) {
    if (record_less<Count>(temp[j - 1], values[i - 1], stats)) {
      record_write<Count>(values[--out], values[--i], stats);
      ++left_streak;
      right_streak = 0;
    } else {
      record_write<Count>(values[--out], temp[--j], stats);
      ++right_streak;
      left_streak = 0;
    }

    if (gallop && i > left.base && j > 0 &&
        left_streak >= spec.gallop_threshold) {
      const std::size_t begin = record_gallop_upper_reverse<Count>(
          left.base, i, temp[j - 1].key,
          [&](std::size_t x) -> const Record<Words>& { return values[x]; }, stats);
      const std::size_t count = i - begin;
      note_record_gallop<Count>(metrics, count);
      while (i > begin) record_write<Count>(values[--out], values[--i], stats);
      left_streak = right_streak = 0;
    } else if (gallop && i > left.base && j > 0 &&
               right_streak >= spec.gallop_threshold) {
      const std::size_t begin = record_gallop_lower_reverse<Count>(
          0, j, values[i - 1].key,
          [&](std::size_t x) -> const Record<Words>& { return temp[x]; }, stats);
      const std::size_t count = j - begin;
      note_record_gallop<Count>(metrics, count);
      while (j > begin) record_write<Count>(values[--out], temp[--j], stats);
      left_streak = right_streak = 0;
    }
  }
  while (j > 0) record_write<Count>(values[--out], temp[--j], stats);
}

template <bool Count, std::size_t Words>
inline void record_merge_adjacent_with_kernel(
    std::vector<Record<Words>>& values, RecordMergeWorkspace<Words>& workspace,
    const AdaptiveRun& left, const AdaptiveRun& right,
    const MergeKernelSpec& spec, RecordMergeKernelMetrics& metrics,
    RecordStats& stats) {
  if (left.base + left.len != right.base || left.len == 0 || right.len == 0) {
    throw std::runtime_error("invalid adjacent record runs");
  }
  if (spec.search == MergeSearchPolicy::gallop && spec.gallop_threshold < 2) {
    throw std::runtime_error("record gallop threshold must be at least 2");
  }
  if constexpr (Count) ++metrics.merge_calls;
  if (spec.buffer == MergeBufferPolicy::full) {
    record_merge_full_buffer<Count>(values, workspace, left, right, spec,
                                    metrics, stats);
  } else if (left.len <= right.len) {
    record_merge_smaller_left<Count>(values, workspace, left, right, spec,
                                     metrics, stats);
  } else {
    record_merge_smaller_right<Count>(values, workspace, left, right, spec,
                                      metrics, stats);
  }
}

template <bool Count, std::size_t Words>
inline std::size_t record_count_run_and_make_ascending(
    std::vector<Record<Words>>& values, std::size_t lo,
    AdaptiveMergeMetrics& metrics, RecordStats& stats) {
  const std::size_t n = values.size();
  if (lo >= n) return 0;
  if (lo + 1 >= n) return 1;

  std::size_t hi = lo + 2;
  if (record_less<Count>(values[lo + 1], values[lo], stats)) {
    while (hi < n && record_less<Count>(values[hi], values[hi - 1], stats)) ++hi;
    for (std::size_t left = lo, right = hi - 1; left < right; ++left, --right) {
      record_swap<Count>(values[left], values[right], stats);
    }
    if constexpr (Count) ++metrics.reversed_runs;
  } else {
    while (hi < n && !record_less<Count>(values[hi], values[hi - 1], stats)) ++hi;
  }
  return hi - lo;
}

template <bool Count, std::size_t Words>
inline void record_binary_extend_sorted_prefix(
    std::vector<Record<Words>>& values, std::size_t lo,
    std::size_t sorted_hi, std::size_t hi, RecordStats& stats) {
  for (std::size_t i = sorted_hi; i < hi; ++i) {
    const Record<Words> value = values[i];
    std::size_t left = lo;
    std::size_t right = i;
    while (left < right) {
      const std::size_t mid = left + (right - left) / 2;
      if (record_less<Count>(value, values[mid], stats)) {
        right = mid;
      } else {
        left = mid + 1;
      }
    }
    for (std::size_t j = i; j > left; --j) {
      record_write<Count>(values[j], values[j - 1], stats);
    }
    record_write<Count>(values[left], value, stats);
  }
}

template <bool Count, std::size_t Words>
inline void record_merge_stack_at(
    std::vector<Record<Words>>& values, RecordMergeWorkspace<Words>& workspace,
    std::vector<AdaptiveRun>& stack, std::size_t index,
    const MergeKernelSpec& kernel, RecordMergeKernelMetrics& kernel_metrics,
    AdaptiveMergeMetrics& merge_metrics, RecordStats& stats) {
  if (index + 1 >= stack.size()) {
    throw std::runtime_error("invalid adaptive record merge stack index");
  }
  const AdaptiveRun left = stack[index];
  const AdaptiveRun right = stack[index + 1];
  if constexpr (Count) {
    merge_metrics.scheduled_merge_cost +=
        static_cast<std::uint64_t>(left.len + right.len);
    ++merge_metrics.merges;
  }
  record_merge_adjacent_with_kernel<Count>(values, workspace, left, right,
                                           kernel, kernel_metrics, stats);
  stack[index] = {left.base, left.len + right.len, 0};
  stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(index + 1));
}

template <bool Count, std::size_t Words>
inline void execute_record_pairwise_policy(
    std::vector<Record<Words>>& values, RecordMergeWorkspace<Words>& workspace,
    std::vector<AdaptiveRun> runs, const MergeKernelSpec& kernel,
    RecordMergeKernelMetrics& kernel_metrics,
    AdaptiveMergeMetrics& merge_metrics, RecordStats& stats) {
  if constexpr (Count) {
    merge_metrics.max_pending_runs = std::max<std::uint64_t>(
        merge_metrics.max_pending_runs, runs.size());
  }
  while (runs.size() > 1) {
    std::vector<AdaptiveRun> next;
    next.reserve((runs.size() + 1) / 2);
    for (std::size_t i = 0; i < runs.size(); i += 2) {
      if (i + 1 == runs.size()) {
        next.push_back(runs[i]);
        continue;
      }
      std::vector<AdaptiveRun> pair = {runs[i], runs[i + 1]};
      record_merge_stack_at<Count>(values, workspace, pair, 0, kernel,
                                   kernel_metrics, merge_metrics, stats);
      next.push_back(pair.front());
    }
    runs.swap(next);
  }
}

template <bool Count, std::size_t Words>
inline void execute_record_timsort_policy(
    std::vector<Record<Words>>& values, RecordMergeWorkspace<Words>& workspace,
    const std::vector<AdaptiveRun>& runs, const MergeKernelSpec& kernel,
    RecordMergeKernelMetrics& kernel_metrics,
    AdaptiveMergeMetrics& merge_metrics, RecordStats& stats) {
  std::vector<AdaptiveRun> stack;
  for (const AdaptiveRun& run : runs) {
    stack.push_back(run);
    if constexpr (Count) {
      merge_metrics.max_pending_runs = std::max<std::uint64_t>(
          merge_metrics.max_pending_runs, stack.size());
    }
    while (stack.size() > 1) {
      std::size_t n = stack.size() - 2;
      const bool first =
          n > 0 && stack[n - 1].len <= stack[n].len + stack[n + 1].len;
      const bool second =
          n > 1 && stack[n - 2].len <= stack[n - 1].len + stack[n].len;
      if (first || second) {
        if (stack[n - 1].len < stack[n + 1].len) --n;
      } else if (stack[n].len > stack[n + 1].len) {
        break;
      }
      record_merge_stack_at<Count>(values, workspace, stack, n, kernel,
                                   kernel_metrics, merge_metrics, stats);
    }
  }
  while (stack.size() > 1) {
    std::size_t n = stack.size() - 2;
    if (n > 0 && stack[n - 1].len < stack[n + 1].len) --n;
    record_merge_stack_at<Count>(values, workspace, stack, n, kernel,
                                 kernel_metrics, merge_metrics, stats);
  }
}

template <bool Count, std::size_t Words>
inline void execute_record_powersort_policy(
    std::vector<Record<Words>>& values, RecordMergeWorkspace<Words>& workspace,
    const std::vector<AdaptiveRun>& runs, const MergeKernelSpec& kernel,
    RecordMergeKernelMetrics& kernel_metrics,
    AdaptiveMergeMetrics& merge_metrics, RecordStats& stats) {
  std::vector<AdaptiveRun> stack;
  const std::size_t total = values.size();
  for (const AdaptiveRun& incoming : runs) {
    if (!stack.empty()) {
      const AdaptiveRun top = stack.back();
      const int power = powerloop(top.base, top.len, incoming.len, total);
      while (stack.size() > 1 && stack[stack.size() - 2].power > power) {
        record_merge_stack_at<Count>(values, workspace, stack,
                                     stack.size() - 2, kernel, kernel_metrics,
                                     merge_metrics, stats);
      }
      stack.back().power = power;
    }
    stack.push_back(incoming);
    if constexpr (Count) {
      merge_metrics.max_pending_runs = std::max<std::uint64_t>(
          merge_metrics.max_pending_runs, stack.size());
    }
  }
  while (stack.size() > 1) {
    std::size_t n = stack.size() - 2;
    if (n > 0 && stack[n - 1].len < stack[n + 1].len) --n;
    record_merge_stack_at<Count>(values, workspace, stack, n, kernel,
                                 kernel_metrics, merge_metrics, stats);
  }
}

template <bool Count, std::size_t Words>
inline void adaptive_record_merge_sort(
    std::vector<Record<Words>>& values, RecordStats& stats,
    MergePolicy merge_policy, MinrunPolicy minrun_policy,
    const MergeKernelSpec& kernel, RecordMergeKernelMetrics& kernel_metrics,
    AdaptiveMergeMetrics& merge_metrics) {
  if constexpr (Count) {
    kernel_metrics = {};
    merge_metrics = {};
  }
  if (values.size() < 2) {
    if constexpr (Count) {
      if (!values.empty()) {
        merge_metrics.natural_runs = 1;
        merge_metrics.effective_runs = 1;
      }
    }
    return;
  }

  const std::size_t fixed_minrun = classic_minrun(values.size());
  BalancedMinrunState balanced(values.size());
  std::vector<AdaptiveRun> runs;
  for (std::size_t lo = 0; lo < values.size();) {
    const std::size_t natural = record_count_run_and_make_ascending<Count>(
        values, lo, merge_metrics, stats);
    if constexpr (Count) ++merge_metrics.natural_runs;
    std::size_t desired = natural;
    if (minrun_policy == MinrunPolicy::classic) {
      desired = fixed_minrun;
    } else if (minrun_policy == MinrunPolicy::balanced) {
      desired = balanced.next();
    }
    const std::size_t remaining = values.size() - lo;
    const std::size_t effective =
        std::min(remaining, std::max(natural, desired));
    if (effective > natural) {
      record_binary_extend_sorted_prefix<Count>(values, lo, lo + natural,
                                                lo + effective, stats);
      if constexpr (Count) {
        merge_metrics.extended_elements +=
            static_cast<std::uint64_t>(effective - natural);
      }
    }
    runs.push_back({lo, effective, 0});
    if constexpr (Count) ++merge_metrics.effective_runs;
    lo += effective;
  }

  if constexpr (Count) {
    const double n = static_cast<double>(values.size());
    for (const auto& run : runs) {
      const double p = static_cast<double>(run.len) / n;
      merge_metrics.run_entropy_bits -= p * std::log2(p);
    }
  }

  RecordMergeWorkspace<Words> workspace;
  switch (merge_policy) {
    case MergePolicy::pairwise:
      execute_record_pairwise_policy<Count>(values, workspace, runs, kernel,
                                            kernel_metrics, merge_metrics, stats);
      break;
    case MergePolicy::timsort_stack:
      execute_record_timsort_policy<Count>(values, workspace, runs, kernel,
                                           kernel_metrics, merge_metrics, stats);
      break;
    case MergePolicy::powersort:
      execute_record_powersort_policy<Count>(values, workspace, runs, kernel,
                                             kernel_metrics, merge_metrics, stats);
      break;
  }
}

inline const std::vector<std::string>& adaptive_record_patterns() {
  static const std::vector<std::string> patterns = {
      "random", "sorted", "reversed", "few_unique", "binary", "all_equal",
      "nearly_sorted", "runs", "descending_runs", "plateau",
      "run_equal32", "run_long_short", "run_power_skew", "run_fibonacci",
      "run_alternating_direction"};
  return patterns;
}

template <std::size_t Words>
inline std::vector<Record<Words>> make_adaptive_records(std::string_view pattern,
                                                        std::size_t n,
                                                        std::uint64_t seed) {
  if (!is_run_shape_pattern(pattern)) return make_records<Words>(pattern, n, seed);
  const auto keys = make_merge_policy_data(pattern, n, seed);
  std::vector<Record<Words>> records(n);
  for (std::size_t i = 0; i < n; ++i) {
    records[i].key = keys[i];
    records[i].ordinal = static_cast<std::uint64_t>(i);
    if constexpr (Words > 0) {
      for (std::size_t word = 0; word < Words; ++word) {
        records[i].payload.words[word] = splitmix64(
            seed ^ (static_cast<std::uint64_t>(i) * 0xD6E8FEB86659FD93ULL) ^
            static_cast<std::uint64_t>(word));
      }
    }
  }
  return records;
}

}  // namespace sortlab
