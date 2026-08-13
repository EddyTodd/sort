cmake_minimum_required(VERSION 3.20)
project(sortlab VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SORTLAB_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(SORTLAB_BUILD_RESEARCH_TOOLS "Build legacy benchmark/research executables pending migration to EddyTodd/bench" OFF)

add_library(sortlab INTERFACE)
add_library(sortlab::sortlab ALIAS sortlab)
target_compile_features(sortlab INTERFACE cxx_std_23)
target_include_directories(sortlab INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(SORTLAB_PUBLIC_HEADERS
  include/sortlab/sort.hpp
  include/sortlab/version.hpp
  include/sortlab/instrumentation.hpp
  include/sortlab/detail.hpp
  include/sortlab/detail/comparison_elementary.hpp
  include/sortlab/detail/comparison_merge.hpp
  include/sortlab/detail/comparison_quick.hpp
  include/sortlab/detail/adaptive_common.hpp
  include/sortlab/detail/adaptive_merger.hpp
  include/sortlab/detail/adaptive_schedule.hpp
  include/sortlab/comparison.hpp
  include/sortlab/instrumented_comparison.hpp
  include/sortlab/adaptive_options.hpp
  include/sortlab/adaptive.hpp
  include/sortlab/stable_inplace.hpp
  include/sortlab/distribution.hpp
  include/sortlab/tiny.hpp
  include/sortlab/metadata.hpp)

install(TARGETS sortlab EXPORT sortlabTargets)
set(SORTLAB_DETAIL_HEADERS
  include/sortlab/detail/comparison_elementary.hpp
  include/sortlab/detail/comparison_merge.hpp
  include/sortlab/detail/comparison_quick.hpp
  include/sortlab/detail/adaptive_common.hpp
  include/sortlab/detail/adaptive_merger.hpp
  include/sortlab/detail/adaptive_schedule.hpp)
list(REMOVE_ITEM SORTLAB_PUBLIC_HEADERS ${SORTLAB_DETAIL_HEADERS})
install(FILES ${SORTLAB_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sortlab)
install(FILES ${SORTLAB_DETAIL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sortlab/detail)
install(EXPORT sortlabTargets NAMESPACE sortlab:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortlab)
configure_package_config_file(
  cmake/sortlabConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/sortlabConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortlab)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/sortlabConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/sortlabConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/sortlabConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortlab)

function(sortlab_apply_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  endif()
endfunction()

function(sortlab_apply_sanitizers target)
  if(SORTLAB_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

function(sortlab_executable target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE sortlab::sortlab)
  sortlab_apply_warnings(${target})
  if(NOT target STREQUAL "sort_alloc")
    sortlab_apply_sanitizers(${target})
  endif()
endfunction()

if(SORTLAB_BUILD_RESEARCH_TOOLS)
  sortlab_executable(sort_lab src/sort_lab.cpp)
  sortlab_executable(sort_records src/sort_records.cpp)
  sortlab_executable(sort_cutoffs src/sort_cutoffs.cpp)
  sortlab_executable(sort_perf src/sort_perf.cpp)
  sortlab_executable(sort_alloc src/sort_alloc.cpp)
  sortlab_executable(sort_tiny src/sort_tiny.cpp)
  sortlab_executable(sort_leaf_hybrids src/sort_leaf_hybrids.cpp)
  sortlab_executable(sort_merge_policies src/sort_merge_policies.cpp)
  sortlab_executable(sort_merge_kernels src/sort_merge_kernels.cpp)
  sortlab_executable(sort_adaptive_records src/sort_adaptive_records.cpp)
  sortlab_executable(adaptive_record_kernel_contract src/adaptive_record_kernel_contract.cpp)
  include(cmake/ExternalBaselines.cmake)
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(sortlab_v1_tests tests/library_tests.cpp)
  target_link_libraries(sortlab_v1_tests PRIVATE sortlab::sortlab)
  sortlab_apply_warnings(sortlab_v1_tests)
  if(NOT MSVC)
    target_compile_options(sortlab_v1_tests PRIVATE -Werror)
  endif()
  sortlab_apply_sanitizers(sortlab_v1_tests)
  add_test(NAME sortlab_v1_tests COMMAND sortlab_v1_tests)

  if(SORTLAB_BUILD_RESEARCH_TOOLS)
    add_test(NAME sort_self_test COMMAND sort_lab --self-test)
    add_test(NAME sort_smoke_benchmark COMMAND sort_lab --trials 2 --warmups 0 --sizes 0,1,32,257 --patterns random,binary,all_equal,organ_pipe,rotated,plateau --algorithms insertion,binary_insertion,heap,natural_merge,merge_insertion_24,quick_3way,dual_pivot,intro,radix_lsd_11,std_sort)
    add_test(NAME record_self_test COMMAND sort_records --self-test)
    add_test(NAME record_smoke_benchmark COMMAND sort_records --trials 2 --warmups 0 --sizes 0,1,32,257 --payload-words 0,7 --patterns few_unique,all_equal,rotated,plateau --algorithms insertion,heap,merge,quick_3way,intro,radix_lsd,std_sort,std_stable_sort)
    add_test(NAME cutoff_self_test COMMAND sort_cutoffs --self-test)
    add_test(NAME perf_self_test COMMAND sort_perf --self-test)
    add_test(NAME allocation_self_test COMMAND sort_alloc --self-test)
    add_test(NAME tiny_kernel_self_test COMMAND sort_tiny --self-test)
    add_test(NAME leaf_hybrid_self_test COMMAND sort_leaf_hybrids --self-test)
    add_test(NAME adaptive_merge_self_test COMMAND sort_merge_policies --self-test)
    add_test(NAME adaptive_merge_kernel_self_test COMMAND sort_merge_kernels --self-test)
    add_test(NAME adaptive_record_self_test COMMAND sort_adaptive_records --self-test)
    add_test(NAME adaptive_record_backward_gallop_contract COMMAND adaptive_record_kernel_contract)
    add_test(NAME tiny_kernel_smoke_benchmark COMMAND sort_tiny --trials 2 --warmups 0 --sizes 2,7,16,31 --patterns random,sorted,few_unique --kernels insertion,binary_insertion,bitonic_network,std_sort)
    add_test(NAME leaf_hybrid_smoke_benchmark COMMAND sort_leaf_hybrids --trials 3 --warmups 0 --sizes 64,257 --patterns random,sorted,few_unique --cutoffs 4,16,32)
    add_test(NAME adaptive_merge_smoke_benchmark COMMAND sort_merge_policies --trials 2 --warmups 0 --sizes 31,64,315 --patterns random,sorted,run_long_short,run_fibonacci --policies pairwise,timsort_stack,powersort --minruns none,classic,balanced)
    add_test(NAME adaptive_merge_kernel_smoke_benchmark COMMAND sort_merge_kernels --trials 2 --warmups 0 --sizes 64,315 --patterns random,run_long_short,run_alternating_direction --gallop-thresholds 4,7)
    add_test(NAME adaptive_record_smoke_benchmark COMMAND sort_adaptive_records --trials 2 --warmups 0 --sizes 31,64,315 --payload-words 0,7,31 --patterns few_unique,binary,run_long_short,run_alternating_direction --policies timsort_stack,powersort --minruns classic,balanced --buffers full,smaller --searches linear,gallop --gallop-thresholds 4,7)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
      add_test(NAME analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze.py --self-test)
      add_test(NAME record_analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_records.py --self-test)
      add_test(NAME crossover_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/crossovers.py --self-test)
      add_test(NAME cutoff_tuning_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/tune_cutoffs.py --self-test)
      add_test(NAME claim_matrix_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/claim_matrix.py --self-test)
      add_test(NAME perf_analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_perf.py --self-test)
      add_test(NAME allocation_analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_alloc.py --self-test)
      add_test(NAME portfolio_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/portfolio.py --self-test)
      add_test(NAME campaign_pipeline_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/campaign.py --self-test)
      add_test(NAME artifact_validation_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_artifacts.py --self-test)
      add_test(NAME evidence_ledger_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/evidence_ledger.py --self-test)
      add_test(NAME research_report_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/report.py --self-test)
      add_test(NAME external_bootstrap_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/bootstrap_external.py --self-test)
      add_test(NAME tiny_analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_tiny.py --self-test)
      add_test(NAME leaf_kernel_tuning_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/tune_leaf_kernels.py --self-test)
      add_test(NAME adaptive_merge_analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_merge_policies.py --self-test)
      add_test(NAME merge_policy_model_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/merge_policy_model.py --self-test)
      add_test(NAME adaptive_merge_kernel_analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_merge_kernels.py --self-test)
      add_test(NAME gallop_tuning_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/tune_gallop.py --self-test)
      add_test(NAME adaptive_record_analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_adaptive_records.py --self-test)
      add_test(NAME adaptive_record_width_analysis_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_record_width_effects.py --self-test)
      add_test(NAME adaptive_record_gallop_tuning_self_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/tune_record_gallop.py --self-test)
    endif()
  endif()
endif()
