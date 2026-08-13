#pragma once
#include <array>
#include <string_view>

namespace sortlab {
enum class stability { stable, unstable };
enum class storage { in_place, logarithmic_stack, linear_buffer, bounded_domain_buffer };
enum class domain { comparison, integral, bounded_integral };

struct algorithm_info {
  std::string_view name;
  std::string_view family;
  stability stable;
  storage auxiliary;
  domain input_domain;
  bool adaptive;
  std::string_view best;
  std::string_view average;
  std::string_view worst;
};

inline constexpr std::array algorithm_catalog{
 algorithm_info{"insertion_sort","insertion",stability::stable,storage::in_place,domain::comparison,true,"O(n)","O(n^2)","O(n^2)"},
 algorithm_info{"binary_insertion_sort","insertion",stability::stable,storage::in_place,domain::comparison,true,"O(n log n) comparisons","O(n^2) moves","O(n^2) moves"},
 algorithm_info{"selection_sort","selection",stability::unstable,storage::in_place,domain::comparison,false,"O(n^2)","O(n^2)","O(n^2)"},
 algorithm_info{"bubble_sort","exchange",stability::stable,storage::in_place,domain::comparison,true,"O(n)","O(n^2)","O(n^2)"},
 algorithm_info{"comb_sort","exchange",stability::unstable,storage::in_place,domain::comparison,true,"gap-dependent","gap-dependent","O(n^2)"},
 algorithm_info{"shell_ciura_sort","shell",stability::unstable,storage::in_place,domain::comparison,true,"gap-dependent","gap-dependent","gap-dependent"},
 algorithm_info{"shell_pratt_sort","shell",stability::unstable,storage::in_place,domain::comparison,true,"gap-dependent","gap-dependent","O(n log^2 n) comparisons"},
 algorithm_info{"heap_sort","heap",stability::unstable,storage::in_place,domain::comparison,false,"O(n log n)","O(n log n)","O(n log n)"},
 algorithm_info{"merge_sort","merge",stability::stable,storage::linear_buffer,domain::comparison,false,"O(n log n)","O(n log n)","O(n log n)"},
 algorithm_info{"merge_bottom_up_sort","merge",stability::stable,storage::linear_buffer,domain::comparison,false,"O(n log n)","O(n log n)","O(n log n)"},
 algorithm_info{"natural_merge_sort","merge-adaptive",stability::stable,storage::linear_buffer,domain::comparison,true,"O(n)","O(n log n)","O(n log n)"},
 algorithm_info{"stable_inplace_merge_sort","stable-low-memory",stability::stable,storage::logarithmic_stack,domain::comparison,false,"O(n log n)","O(n log^2 n)","O(n log^2 n)"},
 algorithm_info{"quick_hoare_sort","quick",stability::unstable,storage::logarithmic_stack,domain::comparison,false,"O(n log n)","O(n log n)","O(n^2)"},
 algorithm_info{"quick_3way_sort","quick",stability::unstable,storage::logarithmic_stack,domain::comparison,true,"O(n) all-equal","O(n log n)","O(n^2)"},
 algorithm_info{"quick_median3_sort","quick",stability::unstable,storage::logarithmic_stack,domain::comparison,false,"O(n log n)","O(n log n)","O(n^2)"},
 algorithm_info{"dual_pivot_sort","quick",stability::unstable,storage::logarithmic_stack,domain::comparison,false,"O(n log n)","O(n log n)","O(n^2)"},
 algorithm_info{"intro_sort","hybrid",stability::unstable,storage::logarithmic_stack,domain::comparison,true,"O(n log n)","O(n log n)","O(n log n)"},
 algorithm_info{"merge_insertion_sort","hybrid",stability::stable,storage::linear_buffer,domain::comparison,true,"O(n)","O(n log n)","O(n log n)"},
 algorithm_info{"quick_insertion_sort","hybrid",stability::unstable,storage::logarithmic_stack,domain::comparison,true,"O(n log n)","O(n log n)","O(n^2)"},
 algorithm_info{"timsort","adaptive-merge",stability::stable,storage::linear_buffer,domain::comparison,true,"O(n)","O(n log n)","O(n log n)"},
 algorithm_info{"powersort","adaptive-merge",stability::stable,storage::linear_buffer,domain::comparison,true,"O(n)","O(n log n)","O(n log n)"},
 algorithm_info{"bitonic_sort","network",stability::unstable,storage::logarithmic_stack,domain::comparison,false,"O(n log^2 n)","O(n log^2 n)","O(n log^2 n)"},
 algorithm_info{"radix_lsd_sort","distribution",stability::stable,storage::linear_buffer,domain::integral,false,"O(w n)","O(w n)","O(w n)"},
 algorithm_info{"radix_msd_sort","distribution",stability::unstable,storage::logarithmic_stack,domain::integral,true,"O(w n)","O(w n)","O(w n)"},
 algorithm_info{"counting_sort","distribution",stability::stable,storage::bounded_domain_buffer,domain::bounded_integral,false,"O(n+k)","O(n+k)","O(n+k)"},
};
} // namespace sortlab
