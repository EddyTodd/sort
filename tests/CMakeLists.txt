include(${PROJECT_SOURCE_DIR}/cmake/HeaderSelfContainment.cmake)
add_public_header_self_containment_target(
  sortlab-header-self-containment
  sortlab
  "${PROJECT_SOURCE_DIR}/include")
sortlab_apply_warnings(sortlab-header-self-containment)

function(sortlab_add_test name source)
  set(target "sortlab-test-${name}")
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE sortlab::sortlab)
  sortlab_configure_private_target(${target})
  add_test(NAME "sortlab.${name}" COMMAND ${target})
endfunction()

sortlab_add_test(library library_tests.cpp)
sortlab_add_test(adaptive-options adaptive_option_contract.cpp)

if(SORTLAB_BUILD_PACKAGE_TESTS AND NOT SORTLAB_ENABLE_SANITIZERS)
  include(${PROJECT_SOURCE_DIR}/cmake/PackageConsumerTest.cmake)
  register_package_consumer_test(
    sortlab.package-consumer
    "${PROJECT_SOURCE_DIR}/tests/package-consumer")
endif()
