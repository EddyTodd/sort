#include "sortlab/adaptive_merge_records.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace sortlab;

namespace {

template <std::size_t Words>
bool backward_gallop_contract() {
  constexpr std::size_t left_size = 64;
  constexpr std::size_t right_size = 16;
  std::vector<Record<Words>> input(left_size + right_size);

  // Both runs are individually sorted.  The shorter right run contains keys
  // duplicated in the left run, so stable equal-key ordering is observable.
  // The large high-key suffix of the left run forces consecutive left wins
  // while merging backward, activating the reverse gallop path.
  for (std::size_t i = 0; i < left_size; ++i) {
    input[i].key = static_cast<Value>(i / 2);
    input[i].ordinal = static_cast<std::uint64_t>(i);
    if constexpr (Words > 0) {
      for (std::size_t word = 0; word < Words; ++word) {
        input[i].payload.words[word] = splitmix64(
            static_cast<std::uint64_t>(i) ^
            (static_cast<std::uint64_t>(word) << 32U));
      }
    }
  }
  for (std::size_t i = 0; i < right_size; ++i) {
    const std::size_t index = left_size + i;
    input[index].key = static_cast<Value>(i / 2);
    input[index].ordinal = static_cast<std::uint64_t>(index);
    if constexpr (Words > 0) {
      for (std::size_t word = 0; word < Words; ++word) {
        input[index].payload.words[word] = splitmix64(
            static_cast<std::uint64_t>(index) ^
            (static_cast<std::uint64_t>(word) << 32U));
      }
    }
  }

  auto output = input;
  RecordMergeWorkspace<Words> workspace;
  RecordMergeKernelMetrics metrics;
  RecordStats stats;
  const MergeKernelSpec kernel{MergeBufferPolicy::smaller_run,
                               MergeSearchPolicy::gallop, 4};
  record_merge_adjacent_with_kernel<true>(
      output, workspace, {0, left_size, 0}, {left_size, right_size, 0},
      kernel, metrics, stats);

  const auto check = verify_records(input, output);
  if (!check.correct || !check.stable) return false;
  if (metrics.gallop_entries == 0 || metrics.gallop_records == 0) return false;
  if (metrics.temp_records_peak != right_size) return false;
  return true;
}

}  // namespace

int main() {
  if (!backward_gallop_contract<0>() || !backward_gallop_contract<7>() ||
      !backward_gallop_contract<31>()) {
    std::cerr << "FAIL: backward adaptive-record gallop/stability contract\n";
    return 1;
  }
  std::cout << "PASS: backward galloping preserves stability across narrow and wide records\n";
  return 0;
}
