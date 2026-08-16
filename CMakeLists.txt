cmake_minimum_required(VERSION 3.23)
project(sortlab VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

option(SORTLAB_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(SORTLAB_WARNINGS_AS_ERRORS "Treat sortlab warnings as errors" ${PROJECT_IS_TOP_LEVEL})
option(SORTLAB_BUILD_TESTS "Build deterministic sortlab correctness tests" ${PROJECT_IS_TOP_LEVEL})
option(SORTLAB_BUILD_EXAMPLES "Build small sortlab usage examples" ${PROJECT_IS_TOP_LEVEL})
option(SORTLAB_BUILD_PACKAGE_TESTS "Run downstream installed-package validation" OFF)
option(SORTLAB_INSTALL "Enable sortlab install/package rules" ${PROJECT_IS_TOP_LEVEL})

if(SORTLAB_BUILD_TESTS)
  include(CTest)
endif()
if(SORTLAB_INSTALL)
  include(CMakePackageConfigHelpers)
endif()
include(cmake/ProjectOptions.cmake)

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

add_library(sortlab INTERFACE)
add_library(sortlab::sortlab ALIAS sortlab)
target_compile_features(sortlab INTERFACE cxx_std_23)
target_include_directories(sortlab INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_sources(sortlab INTERFACE
  FILE_SET public_headers TYPE HEADERS
  BASE_DIRS ${PROJECT_SOURCE_DIR}/include
  FILES ${SORTLAB_PUBLIC_HEADERS})

if(SORTLAB_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BUILD_TESTING AND SORTLAB_BUILD_TESTS)
  add_subdirectory(tests)
endif()

if(SORTLAB_INSTALL)
  include(cmake/Install.cmake)
endif()
