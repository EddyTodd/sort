#include <sortlab/sort.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct Record {
  int key{};
  int ordinal{};
  std::string payload;
};

struct MoveOnly {
  int key{};
  explicit MoveOnly(int value) : key(value) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) noexcept = default;
  MoveOnly& operator=(MoveOnly&&) noexcept = default;
};

template <class Sort>
void exercise_int_sort(Sort sort) {
  const std::vector<std::vector<int>> cases = {
      {}, {1}, {2, 1}, {1, 1, 1, 1}, {1, 2, 3, 4}, {4, 3, 2, 1},
      {3, 1, 2, 3, 0, -1, 2},
      {std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max(), -1, 1}};
  for (auto input : cases) {
    auto expected = input;
    std::sort(expected.begin(), expected.end());
    sort(input);
    require(input == expected, "deterministic integer case failed");
  }

  std::mt19937 rng(0xC0FFEEu);
  for (std::size_t n = 0; n < 160; ++n) {
    std::vector<int> input(n);
    for (auto& value : input) value = static_cast<int>(rng() % 33U) - 16;
    auto expected = input;
    std::sort(expected.begin(), expected.end());
    sort(input);
    require(input == expected, "random duplicate-heavy case failed");
  }
}

template <class Sort>
void exercise_stable(Sort sort) {
  std::vector<Record> input;
  for (int i = 0; i < 240; ++i) input.push_back({i % 11, i, "payload-" + std::to_string(i)});
  sort(input);
  for (std::size_t i = 1; i < input.size(); ++i) {
    require(input[i - 1].key <= input[i].key, "stable algorithm did not sort records");
    if (input[i - 1].key == input[i].key)
      require(input[i - 1].ordinal < input[i].ordinal, "stable algorithm changed equal-key order");
  }
}

void test_comparison_catalog() {
  exercise_int_sort([](auto& v) { sortlab::insertion_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::binary_insertion_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::selection_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::bubble_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::comb_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::shell_ciura_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::shell_pratt_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::heap_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::merge_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::merge_bottom_up_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::natural_merge_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::stable_inplace_merge_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::quick_hoare_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::quick_3way_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::quick_median3_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::dual_pivot_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::intro_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::merge_insertion_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::quick_insertion_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::timsort(v); });
  exercise_int_sort([](auto& v) { sortlab::powersort(v); });
  exercise_int_sort([](auto& v) { sortlab::bitonic_sort(v); });
}

void test_comparators_projections_and_stability() {
  std::vector<int> descending = {1, 5, 2, 4, 3, 5};
  sortlab::intro_sort(descending, 24, std::ranges::greater{});
  require(std::is_sorted(descending.begin(), descending.end(), std::greater<>{}),
          "custom descending comparator failed");

  std::array<int, 9> array_range = {7, 1, 4, 1, 9, 2, 8, 3, 0};
  sortlab::heap_sort(array_range);
  require(std::ranges::is_sorted(array_range), "non-vector random-access range failed");

  exercise_stable([](auto& v) { sortlab::insertion_sort(v.begin(), v.end(), std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::binary_insertion_sort(v.begin(), v.end(), std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::bubble_sort(v.begin(), v.end(), std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::merge_sort(v.begin(), v.end(), std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::merge_bottom_up_sort(v.begin(), v.end(), std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::natural_merge_sort(v.begin(), v.end(), std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::stable_inplace_merge_sort(v.begin(), v.end(), std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::merge_insertion_sort(v.begin(), v.end(), 24, std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::timsort(v, std::ranges::less{}, &Record::key); });
  exercise_stable([](auto& v) { sortlab::powersort(v, std::ranges::less{}, &Record::key); });
}

void test_move_only_contracts() {
  auto make = [] {
    std::vector<MoveOnly> values;
    for (int value : {5, 1, 3, 1, 9, -1, 2, 2}) values.emplace_back(value);
    return values;
  };
  const auto sorted = [](const auto& values) {
    return std::is_sorted(values.begin(), values.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
  };

  auto insertion = make();
  sortlab::insertion_sort(insertion.begin(), insertion.end(), std::ranges::less{}, &MoveOnly::key);
  require(sorted(insertion), "move-only insertion failed");

  auto heap = make();
  sortlab::heap_sort(heap.begin(), heap.end(), std::ranges::less{}, &MoveOnly::key);
  require(sorted(heap), "move-only heap failed");

  auto merge = make();
  sortlab::merge_sort(merge.begin(), merge.end(), std::ranges::less{}, &MoveOnly::key);
  require(sorted(merge), "move-only merge failed");

  auto stable_inplace = make();
  sortlab::stable_inplace_merge_sort(stable_inplace.begin(), stable_inplace.end(), std::ranges::less{}, &MoveOnly::key);
  require(sorted(stable_inplace), "move-only stable in-place merge failed");

  auto tim = make();
  sortlab::timsort(tim.begin(), tim.end(), std::ranges::less{}, &MoveOnly::key);
  require(sorted(tim), "move-only TimSort failed");

  auto power = make();
  sortlab::powersort(power.begin(), power.end(), std::ranges::less{}, &MoveOnly::key);
  require(sorted(power), "move-only Powersort failed");
}

void test_adaptive_pathologies_and_dynamic_gallop() {
  std::vector<Record> runs;
  int ordinal = 0;
  const std::array<int, 13> lengths = {1, 64, 2, 63, 3, 62, 5, 97, 8, 159, 13, 258, 21};
  int base = 100000;
  for (std::size_t r = 0; r < lengths.size(); ++r) {
    const int len = lengths[r];
    for (int i = 0; i < len; ++i) {
      const int key = (r % 2 == 0) ? base + i : -base + i;
      runs.push_back({key, ordinal, "r" + std::to_string(r) + ":" + std::to_string(i)});
      ++ordinal;
    }
    base += 1000;
  }
  auto tim = runs;
  sortlab::timsort(tim, std::ranges::less{}, &Record::key);
  require(std::ranges::is_sorted(tim, std::ranges::less{}, &Record::key), "pathological TimSort run stack failed");
  auto power = runs;
  sortlab::powersort(power, std::ranges::less{}, &Record::key);
  require(std::ranges::is_sorted(power, std::ranges::less{}, &Record::key), "pathological Powersort run stack failed");

  std::vector<int> gallop_case;
  gallop_case.reserve(512);
  for (int i = 0; i < 256; ++i) gallop_case.push_back(i);
  for (int i = 0; i < 100; ++i) gallop_case.push_back(-10000 + i);
  for (int i = 0; i < 156; ++i) gallop_case.push_back(1000 + i);
  sortlab::operation_counts counts;
  sortlab::counting_observer observer(counts);
  sortlab::instrumented::timsort(gallop_case.begin(), gallop_case.end(), observer);
  require(std::is_sorted(gallop_case.begin(), gallop_case.end()), "instrumented TimSort failed");
  require(counts.gallop_entries > 0, "TimSort did not enter galloping on forced streaks");
  require(counts.gallop_threshold_updates > 0, "dynamic min_gallop did not adapt");
}

void test_adaptive_merge_directions() {
  auto make_record = [](int key, int ordinal) { return Record{key, ordinal, "d" + std::to_string(ordinal)}; };
  std::vector<Record> forward;
  int ordinal = 0;
  for (int key : {1, 2, 2, 4}) forward.push_back(make_record(key, ordinal++));
  for (int key : {2, 2, 3, 5, 6, 7, 8}) forward.push_back(make_record(key, ordinal++));
  {
    sortlab::operation_counts counts;
    sortlab::counting_observer observer(counts);
    sortlab::detail::operations ops(std::ranges::less{}, &Record::key, observer);
    sortlab::detail::adaptive_merger merger(forward.begin(), ops, 2);
    merger.merge({0, 4, 0}, {4, 7, 0});
    require(std::ranges::is_sorted(forward, std::ranges::less{}, &Record::key), "forward adaptive merge did not sort");
    for (std::size_t i = 1; i < forward.size(); ++i)
      if (forward[i - 1].key == forward[i].key)
        require(forward[i - 1].ordinal < forward[i].ordinal, "forward adaptive merge violated stability");
  }

  std::vector<Record> backward;
  ordinal = 0;
  for (int key = 0; key < 96; ++key) backward.push_back(make_record(key / 3, ordinal++));
  for (int key = 24; key < 40; ++key) backward.push_back(make_record(key / 2, ordinal++));
  {
    sortlab::operation_counts counts;
    sortlab::counting_observer observer(counts);
    sortlab::detail::operations ops(std::ranges::less{}, &Record::key, observer);
    sortlab::detail::adaptive_merger merger(backward.begin(), ops, 2);
    merger.merge({0, 96, 0}, {96, 16, 0});
    require(std::ranges::is_sorted(backward, std::ranges::less{}, &Record::key), "backward adaptive merge did not sort");
    for (std::size_t i = 1; i < backward.size(); ++i)
      if (backward[i - 1].key == backward[i].key)
        require(backward[i - 1].ordinal < backward[i].ordinal, "backward adaptive merge violated stability");
    require(counts.gallop_entries > 0, "backward adaptive merge did not exercise galloping");
  }
}

void test_distribution_domains() {
  exercise_int_sort([](auto& v) { sortlab::radix_lsd_sort(v); });
  exercise_int_sort([](auto& v) { sortlab::radix_msd_sort(v); });

  std::vector<std::int64_t> extremes = {
      std::numeric_limits<std::int64_t>::min(), -1, 0, 1,
      std::numeric_limits<std::int64_t>::max(), -17, 17};
  auto expected = extremes;
  std::sort(expected.begin(), expected.end());
  for (unsigned bits : {1U, 4U, 7U, 8U, 11U}) {
    auto values = extremes;
    sortlab::radix_lsd_sort(values, bits);
    require(values == expected, "LSD radix digit-width boundary failed");
  }
  for (unsigned bits : {1U, 4U, 8U}) {
    auto values = extremes;
    sortlab::radix_msd_sort(values, bits);
    require(values == expected, "MSD radix digit-width boundary failed");
  }

  std::vector<int> bounded = {5, -2, 5, 0, -2, 3, 3, 3};
  auto bounded_expected = bounded;
  std::sort(bounded_expected.begin(), bounded_expected.end());
  sortlab::counting_sort(bounded, 1000);
  require(bounded == bounded_expected, "counting sort bounded domain failed");

  std::vector<std::uint8_t> narrow = {255, 0, 7, 7, 1, 128, 2};
  auto narrow_expected = narrow;
  std::sort(narrow_expected.begin(), narrow_expected.end());
  sortlab::counting_sort(narrow);
  require(narrow == narrow_expected, "counting sort narrow unsigned domain failed");

  bool rejected = false;
  try {
    std::vector<int> huge = {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
    sortlab::counting_sort(huge, 1024);
  } catch (const std::length_error&) {
    rejected = true;
  }
  require(rejected, "counting sort did not reject oversized domain");

  bool zero_domain_rejected = false;
  try {
    std::vector<int> values = {1, 0};
    sortlab::counting_sort(values, 0);
  } catch (const std::invalid_argument&) {
    zero_domain_rejected = true;
  }
  require(zero_domain_rejected, "counting sort accepted zero max_domain");
}

void test_instrumentation_and_metadata() {
  sortlab::operation_counts counts;
  sortlab::counting_observer observer(counts);
  std::vector<int> values = {4, 1, 3, 2};
  sortlab::instrumented::merge_sort(values.begin(), values.end(), observer);
  require(std::is_sorted(values.begin(), values.end()), "instrumented algorithm failed");
  require(counts.comparisons > 0 && counts.writes > 0, "instrumentation did not observe operations");
  std::vector<int> network = {9, 1, 7, 2, 6, 3, 5, 4};
  sortlab::instrumented::bitonic_sort(network.begin(), network.end(), observer);
  require(std::is_sorted(network.begin(), network.end()), "instrumented bitonic sort failed");

  std::unordered_set<std::string_view> names;
  for (const auto& info : sortlab::algorithm_catalog) {
    require(!info.name.empty() && !info.family.empty(), "metadata has empty name/family");
    require(names.insert(info.name).second, "duplicate algorithm metadata name");
    require(!info.best.empty() && !info.average.empty() && !info.worst.empty(), "metadata complexity missing");
  }
  require(names.contains("timsort") && names.contains("powersort") &&
              names.contains("counting_sort") && names.contains("radix_msd_sort") &&
              names.contains("stable_inplace_merge_sort"),
          "v1 representative mechanisms missing from metadata");
}

}  // namespace

int main() {
  try {
    test_comparison_catalog();
    test_comparators_projections_and_stability();
    test_move_only_contracts();
    test_adaptive_pathologies_and_dynamic_gallop();
    test_adaptive_merge_directions();
    test_distribution_domains();
    test_instrumentation_and_metadata();
    std::cout << "PASS: sortlab v1 generic algorithm library\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
