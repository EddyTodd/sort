cmake_minimum_required(VERSION 3.23)
project(sortlab VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

option(SORTLAB_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(SORTLAB_WARNINGS_AS_ERRORS "Treat sortlab warnings as errors" ${PROJECT_IS_TOP_LEVEL})
option(SORTLAB_BUILD_TESTS "Build deterministic sortlab correctness tests" ${PROJECT_IS_TOP_LEVEL})

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
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_sources(sortlab INTERFACE
  FILE_SET public_headers TYPE HEADERS
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
  FILES ${SORTLAB_PUBLIC_HEADERS})

install(TARGETS sortlab EXPORT sortlabTargets
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
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
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/sortlab)

function(sortlab_apply_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
    if(SORTLAB_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(SORTLAB_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

function(sortlab_apply_sanitizers target)
  if(SORTLAB_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

if(BUILD_TESTING AND SORTLAB_BUILD_TESTS)
  include(cmake/HeaderSelfContainment.cmake)
  add_public_header_self_containment_target(
    sortlab-header-self-containment
    sortlab
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
  sortlab_apply_warnings(sortlab-header-self-containment)

  add_executable(sortlab_v1_tests tests/library_tests.cpp)
  target_link_libraries(sortlab_v1_tests PRIVATE sortlab::sortlab)
  set_target_properties(sortlab_v1_tests PROPERTIES CXX_EXTENSIONS OFF)
  sortlab_apply_warnings(sortlab_v1_tests)
  sortlab_apply_sanitizers(sortlab_v1_tests)
  add_test(NAME sortlab_v1_tests COMMAND sortlab_v1_tests)

  add_executable(sortlab_adaptive_option_contract tests/adaptive_option_contract.cpp)
  target_link_libraries(sortlab_adaptive_option_contract PRIVATE sortlab::sortlab)
  set_target_properties(sortlab_adaptive_option_contract PROPERTIES CXX_EXTENSIONS OFF)
  sortlab_apply_warnings(sortlab_adaptive_option_contract)
  sortlab_apply_sanitizers(sortlab_adaptive_option_contract)
  add_test(NAME sortlab_adaptive_option_contract COMMAND sortlab_adaptive_option_contract)

  if(NOT SORTLAB_ENABLE_SANITIZERS)
    include(cmake/PackageConsumerTest.cmake)
    register_package_consumer_test(
      sortlab.package-consumer
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/package-consumer")
  endif()
endif()
