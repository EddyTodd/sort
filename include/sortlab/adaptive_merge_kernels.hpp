#pragma once

#include "sortlab/adaptive_merge.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sortlab {

enum class MergeBufferPolicy { full, smaller_run };
enum class MergeSearchPolicy { linear, gallop };

inline const char* merge_buffer_policy_name(MergeBufferPolicy policy) {
  return policy == MergeBufferPolicy::full ? "full" : "smaller";
}
inline const char* merge_search_policy_name(MergeSearchPolicy policy) {
  return policy == MergeSearchPolicy::linear ? "linear" : "gallop";
}

struct MergeKernelSpec {
  MergeBufferPolicy buffer = MergeBufferPolicy::full;
  MergeSearchPolicy search = MergeSearchPolicy::linear;
  std::size_t gallop_threshold = 0;
};

inline std::string merge_kernel_name(const MergeKernelSpec& spec) {
  std::string out = std::string(merge_buffer_policy_name(spec.buffer)) + "_" +
                    merge_search_policy_name(spec.search);
  if (spec.search == MergeSearchPolicy::gallop) {
    out += "_" + std::to_string(spec.gallop_threshold);
  }
  return out;
}

struct MergeKernelMetrics {
  std::uint64_t merge_calls = 0;
  std::uint64_t gallop_entries = 0;
  std::uint64_t gallop_elements = 0;
  std::uint64_t temp_elements_peak = 0;
  std::uint64_t temp_capacity_peak = 0;
  std::uint64_t temp_elements_copied = 0;
};

struct MergeKernelWorkspace {
  std::vector<Value> storage;
};

template <bool Count>
inline void ensure_kernel_workspace(MergeKernelWorkspace& workspace,
                                    std::size_t required,
                                    MergeKernelMetrics& metrics) {
  if (workspace.storage.size() < required) workspace.storage.resize(required);
  if constexpr (Count) {
    metrics.temp_elements_peak = std::max<std::uint64_t>(metrics.temp_elements_peak, required);
    metrics.temp_capacity_peak = std::max<std::uint64_t>(
        metrics.temp_capacity_peak, workspace.storage.capacity());
  }
}

template <bool Count, class Accessor>
inline std::size_t counted_lower_bound(std::size_t lo, std::size_t hi, Value key,
                                       Accessor&& at, Stats& stats) {
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2;
    if (lessv<Count>(at(mid), key, stats)) lo = mid + 1;
    else hi = mid;
  }
  return lo;
}

template <bool Count, class Accessor>
inline std::size_t counted_upper_bound(std::size_t lo, std::size_t hi, Value key,
                                       Accessor&& at, Stats& stats) {
  while (lo < hi) {
    const std::size_t mid = lo + (hi - lo) / 2;
    if (!lessv<Count>(key, at(mid), stats)) lo = mid + 1;
    else hi = mid;
  }
  return lo;
}

template <bool Count>
inline void note_gallop(MergeKernelMetrics& metrics, std::size_t elements) {
  if constexpr (Count) {
    ++metrics.gallop_entries;
    metrics.gallop_elements += static_cast<std::uint64_t>(elements);
  }
}

template <bool Count>
inline void merge_full_buffer(std::vector<Value>& values,
                              MergeKernelWorkspace& workspace,
                              const AdaptiveRun& left, const AdaptiveRun& right,
                              const MergeKernelSpec& spec,
                              MergeKernelMetrics& metrics, Stats& stats) {
  const std::size_t total = left.len + right.len;
  ensure_kernel_workspace<Count>(workspace, total, metrics);
  auto& temp = workspace.storage;
  for (std::size_t x = 0; x < left.len; ++x) {
    writev<Count>(temp[x], values[left.base + x], stats);
  }
  for (std::size_t x = 0; x < right.len; ++x) {
    writev<Count>(temp[left.len + x], values[right.base + x], stats);
  }
  if constexpr (Count) metrics.temp_elements_copied += static_cast<std::uint64_t>(total);

  std::size_t i = 0;
  std::size_t j = left.len;
  const std::size_t left_end = left.len;
  const std::size_t right_end = total;
  std::size_t out = left.base;
  std::size_t left_streak = 0;
  std::size_t right_streak = 0;
  const bool gallop = spec.search == MergeSearchPolicy::gallop;

  while (i < left_end && j < right_end) {
    if (lessv<Count>(temp[j], temp[i], stats)) {
      writev<Count>(values[out++], temp[j++], stats);
      ++right_streak;
      left_streak = 0;
    } else {
      writev<Count>(values[out++], temp[i++], stats);
      ++left_streak;
      right_streak = 0;
    }

    if (gallop && i < left_end && j < right_end &&
        left_streak >= spec.gallop_threshold) {
      const std::size_t end = counted_upper_bound<Count>(
          i, left_end, temp[j], [&](std::size_t x) { return temp[x]; }, stats);
      const std::size_t count = end - i;
      note_gallop<Count>(metrics, count);
      while (i < end) writev<Count>(values[out++], temp[i++], stats);
      left_streak = right_streak = 0;
    } else if (gallop && i < left_end && j < right_end &&
               right_streak >= spec.gallop_threshold) {
      const std::size_t end = counted_lower_bound<Count>(
          j, right_end, temp[i], [&](std::size_t x) { return temp[x]; }, stats);
      const std::size_t count = end - j;
      note_gallop<Count>(metrics, count);
      while (j < end) writev<Count>(values[out++], temp[j++], stats);
      left_streak = right_streak = 0;
    }
  }
  while (i < left_end) writev<Count>(values[out++], temp[i++], stats);
  while (j < right_end) writev<Count>(values[out++], temp[j++], stats);
}

template <bool Count>
inline void merge_smaller_left(std::vector<Value>& values,
                               MergeKernelWorkspace& workspace,
                               const AdaptiveRun& left, const AdaptiveRun& right,
                               const MergeKernelSpec& spec,
                               MergeKernelMetrics& metrics, Stats& stats) {
  ensure_kernel_workspace<Count>(workspace, left.len, metrics);
  auto& temp = workspace.storage;
  for (std::size_t x = 0; x < left.len; ++x) {
    writev<Count>(temp[x], values[left.base + x], stats);
  }
  if constexpr (Count) metrics.temp_elements_copied += static_cast<std::uint64_t>(left.len);

  std::size_t i = 0;
  std::size_t j = right.base;
  std::size_t out = left.base;
  const std::size_t hi = right.base + right.len;
  std::size_t left_streak = 0;
  std::size_t right_streak = 0;
  const bool gallop = spec.search == MergeSearchPolicy::gallop;

  while (i < left.len && j < hi) {
    if (lessv<Count>(values[j], temp[i], stats)) {
      writev<Count>(values[out++], values[j++], stats);
      ++right_streak;
      left_streak = 0;
    } else {
      writev<Count>(values[out++], temp[i++], stats);
      ++left_streak;
      right_streak = 0;
    }
    if (gallop && i < left.len && j < hi && left_streak >= spec.gallop_threshold) {
      const std::size_t end = counted_upper_bound<Count>(
          i, left.len, values[j], [&](std::size_t x) { return temp[x]; }, stats);
      const std::size_t count = end - i;
      note_gallop<Count>(metrics, count);
      while (i < end) writev<Count>(values[out++], temp[i++], stats);
      left_streak = right_streak = 0;
    } else if (gallop && i < left.len && j < hi &&
               right_streak >= spec.gallop_threshold) {
      const std::size_t end = counted_lower_bound<Count>(
          j, hi, temp[i], [&](std::size_t x) { return values[x]; }, stats);
      const std::size_t count = end - j;
      note_gallop<Count>(metrics, count);
      while (j < end) writev<Count>(values[out++], values[j++], stats);
      left_streak = right_streak = 0;
    }
  }
  while (i < left.len) writev<Count>(values[out++], temp[i++], stats);
}

template <bool Count>
inline void merge_smaller_right(std::vector<Value>& values,
                                MergeKernelWorkspace& workspace,
                                const AdaptiveRun& left, const AdaptiveRun& right,
                                const MergeKernelSpec& spec,
                                MergeKernelMetrics& metrics, Stats& stats) {
  ensure_kernel_workspace<Count>(workspace, right.len, metrics);
  auto& temp = workspace.storage;
  for (std::size_t x = 0; x < right.len; ++x) {
    writev<Count>(temp[x], values[right.base + x], stats);
  }
  if constexpr (Count) metrics.temp_elements_copied += static_cast<std::uint64_t>(right.len);

  std::size_t i = left.base + left.len;
  std::size_t j = right.len;
  std::size_t out = right.base + right.len;
  std::size_t left_streak = 0;
  std::size_t right_streak = 0;
  const bool gallop = spec.search == MergeSearchPolicy::gallop;

  while (i > left.base && j > 0) {
    if (lessv<Count>(temp[j - 1], values[i - 1], stats)) {
      writev<Count>(values[--out], values[--i], stats);
      ++left_streak;
      right_streak = 0;
    } else {
      writev<Count>(values[--out], temp[--j], stats);
      ++right_streak;
      left_streak = 0;
    }
    if (gallop && i > left.base && j > 0 && left_streak >= spec.gallop_threshold) {
      const std::size_t begin = counted_upper_bound<Count>(
          left.base, i, temp[j - 1], [&](std::size_t x) { return values[x]; }, stats);
      const std::size_t count = i - begin;
      note_gallop<Count>(metrics, count);
      while (i > begin) writev<Count>(values[--out], values[--i], stats);
      left_streak = right_streak = 0;
    } else if (gallop && i > left.base && j > 0 &&
               right_streak >= spec.gallop_threshold) {
      const std::size_t begin = counted_lower_bound<Count>(
          0, j, values[i - 1], [&](std::size_t x) { return temp[x]; }, stats);
      const std::size_t count = j - begin;
      note_gallop<Count>(metrics, count);
      while (j > begin) writev<Count>(values[--out], temp[--j], stats);
      left_streak = right_streak = 0;
    }
  }
  while (j > 0) writev<Count>(values[--out], temp[--j], stats);
}

template <bool Count>
inline void merge_adjacent_with_kernel(std::vector<Value>& values,
                                       MergeKernelWorkspace& workspace,
                                       const AdaptiveRun& left,
                                       const AdaptiveRun& right,
                                       const MergeKernelSpec& spec,
                                       MergeKernelMetrics& metrics,
                                       Stats& stats) {
  if (left.base + left.len != right.base || left.len == 0 || right.len == 0) {
    throw std::runtime_error("invalid adjacent runs for merge-kernel experiment");
  }
  if (spec.search == MergeSearchPolicy::gallop && spec.gallop_threshold < 2) {
    throw std::runtime_error("gallop threshold must be at least 2");
  }
  if constexpr (Count) ++metrics.merge_calls;
  if (spec.buffer == MergeBufferPolicy::full) {
    merge_full_buffer<Count>(values, workspace, left, right, spec, metrics, stats);
  } else if (left.len <= right.len) {
    merge_smaller_left<Count>(values, workspace, left, right, spec, metrics, stats);
  } else {
    merge_smaller_right<Count>(values, workspace, left, right, spec, metrics, stats);
  }
}

template <bool Count>
inline std::vector<AdaptiveRun> balanced_effective_runs(std::vector<Value>& values,
                                                       AdaptiveMergeMetrics& metrics,
                                                       Stats& stats) {
  BalancedMinrunState balanced(values.size());
  std::vector<AdaptiveRun> runs;
  for (std::size_t lo = 0; lo < values.size();) {
    const std::size_t natural = count_run_and_make_ascending<Count>(values, lo, metrics, stats);
    if constexpr (Count) ++metrics.natural_runs;
    const std::size_t desired = balanced.next();
    const std::size_t remaining = values.size() - lo;
    const std::size_t effective = std::min(remaining, std::max(natural, desired));
    if (effective > natural) {
      binary_extend_sorted_prefix<Count>(values, lo, lo + natural, lo + effective, stats);
      if constexpr (Count) metrics.extended_elements +=
          static_cast<std::uint64_t>(effective - natural);
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
  return runs;
}

template <bool Count>
inline void merge_kernel_stack_at(std::vector<Value>& values,
                                  MergeKernelWorkspace& workspace,
                                  std::vector<AdaptiveRun>& stack,
                                  std::size_t index,
                                  const MergeKernelSpec& spec,
                                  MergeKernelMetrics& kernel_metrics,
                                  AdaptiveMergeMetrics& merge_metrics,
                                  Stats& stats) {
  const AdaptiveRun left = stack.at(index);
  const AdaptiveRun right = stack.at(index + 1);
  if constexpr (Count) {
    merge_metrics.scheduled_merge_cost += static_cast<std::uint64_t>(left.len + right.len);
    ++merge_metrics.merges;
  }
  merge_adjacent_with_kernel<Count>(values, workspace, left, right, spec,
                                    kernel_metrics, stats);
  stack[index] = {left.base, left.len + right.len, 0};
  stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(index + 1));
}

template <bool Count>
inline void powersort_balanced_with_kernel(std::vector<Value>& values, Stats& stats,
                                           const MergeKernelSpec& spec,
                                           MergeKernelMetrics& kernel_metrics,
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

  auto runs = balanced_effective_runs<Count>(values, merge_metrics, stats);
  MergeKernelWorkspace workspace;
  std::vector<AdaptiveRun> stack;
  const std::size_t total = values.size();
  for (const AdaptiveRun& incoming : runs) {
    if (!stack.empty()) {
      const AdaptiveRun top = stack.back();
      const int power = powerloop(top.base, top.len, incoming.len, total);
      while (stack.size() > 1 && stack[stack.size() - 2].power > power) {
        merge_kernel_stack_at<Count>(values, workspace, stack, stack.size() - 2,
                                     spec, kernel_metrics, merge_metrics, stats);
      }
      stack.back().power = power;
    }
    stack.push_back(incoming);
    if constexpr (Count) merge_metrics.max_pending_runs = std::max<std::uint64_t>(
        merge_metrics.max_pending_runs, stack.size());
  }
  while (stack.size() > 1) {
    std::size_t index = stack.size() - 2;
    if (index > 0 && stack[index - 1].len < stack[index + 1].len) --index;
    merge_kernel_stack_at<Count>(values, workspace, stack, index, spec,
                                 kernel_metrics, merge_metrics, stats);
  }
}

}  // namespace sortlab
