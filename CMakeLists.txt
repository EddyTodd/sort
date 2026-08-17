cmake_minimum_required(VERSION 3.23)
project(sortlab VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

option(SORTLAB_BUILD_TESTS "Build correctness tests" ${PROJECT_IS_TOP_LEVEL})
option(SORTLAB_BUILD_EXAMPLES "Build usage examples" ${PROJECT_IS_TOP_LEVEL})
option(SORTLAB_INSTALL "Enable install/package rules" ${PROJECT_IS_TOP_LEVEL})

add_library(sortlab INTERFACE)
add_library(sortlab::sortlab ALIAS sortlab)
target_compile_features(sortlab INTERFACE cxx_std_23)
target_include_directories(sortlab INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(SORTLAB_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(SORTLAB_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(SORTLAB_INSTALL)
  include(CMakePackageConfigHelpers)

  install(TARGETS sortlab EXPORT sortlabTargets)
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(EXPORT sortlabTargets
    FILE sortlabTargets.cmake
    NAMESPACE sortlab::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortlab)

  file(WRITE "${PROJECT_BINARY_DIR}/sortlabConfig.cmake"
    "include(\"\${CMAKE_CURRENT_LIST_DIR}/sortlabTargets.cmake\")\n")
  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/sortlabConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
  install(FILES
    "${PROJECT_BINARY_DIR}/sortlabConfig.cmake"
    "${PROJECT_BINARY_DIR}/sortlabConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortlab)
  install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/sortlab)
endif()
