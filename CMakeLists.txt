cmake_minimum_required(VERSION 3.20)
project(sort_lab VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SORTLAB_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

function(sortlab_executable target source)
  add_executable(${target} ${source})
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(SORTLAB_ENABLE_SANITIZERS AND NOT target STREQUAL "sort_alloc")
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
endfunction()

sortlab_executable(sort_lab src/sort_lab.cpp)
sortlab_executable(sort_records src/sort_records.cpp)
sortlab_executable(sort_cutoffs src/sort_cutoffs.cpp)
sortlab_executable(sort_perf src/sort_perf.cpp)
sortlab_executable(sort_alloc src/sort_alloc.cpp)

include(CTest)
include(cmake/ExternalBaselines.cmake)

if(BUILD_TESTING)
  add_test(NAME sort_self_test COMMAND sort_lab --self-test)
  add_test(
    NAME sort_smoke_benchmark
    COMMAND sort_lab --trials 2 --warmups 0 --sizes 0,1,32,257
            --patterns random,binary,all_equal,organ_pipe,rotated,plateau
            --algorithms "insertion,binary_insertion,heap,natural_merge,merge_insertion_24,quick_3way,dual_pivot,intro,radix_lsd_11,std_sort")

  add_test(NAME record_self_test COMMAND sort_records --self-test)
  add_test(
    NAME record_smoke_benchmark
    COMMAND sort_records --trials 2 --warmups 0 --sizes 0,1,32,257 --payload-words 0,7
            --patterns few_unique,all_equal,rotated,plateau
            --algorithms insertion,heap,merge,quick_3way,intro,radix_lsd,std_sort,std_stable_sort)

  add_test(NAME cutoff_self_test COMMAND sort_cutoffs --self-test)
  add_test(NAME perf_self_test COMMAND sort_perf --self-test)
  add_test(NAME allocation_self_test COMMAND sort_alloc --self-test)

  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME analysis_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze.py --self-test)
    add_test(NAME record_analysis_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_records.py --self-test)
    add_test(NAME crossover_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/crossovers.py --self-test)
    add_test(NAME cutoff_tuning_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/tune_cutoffs.py --self-test)
    add_test(NAME claim_matrix_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/claim_matrix.py --self-test)
    add_test(NAME perf_analysis_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_perf.py --self-test)
    add_test(NAME allocation_analysis_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_alloc.py --self-test)
    add_test(NAME portfolio_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/portfolio.py --self-test)
    add_test(NAME campaign_pipeline_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/campaign.py --self-test)
    add_test(NAME artifact_validation_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_artifacts.py --self-test)
    add_test(NAME evidence_ledger_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/evidence_ledger.py --self-test)
    add_test(NAME research_report_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/report.py --self-test)
    add_test(NAME external_bootstrap_self_test
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/bootstrap_external.py --self-test)
  endif()
endif()
