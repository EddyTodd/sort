function(add_sortlab_test name source)
  add_executable(sortlab-test-${name} ${source})
  target_link_libraries(sortlab-test-${name} PRIVATE sortlab::sortlab)
  add_test(NAME sortlab.${name} COMMAND sortlab-test-${name})
endfunction()

add_sortlab_test(library test_library.cpp)
add_sortlab_test(adaptive-options test_adaptive_options.cpp)
add_sortlab_test(differential test_differential.cpp)
