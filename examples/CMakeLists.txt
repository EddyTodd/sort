add_executable(sortlab-example-basic basic.cpp)
target_link_libraries(sortlab-example-basic PRIVATE sortlab::sortlab)
sortlab_configure_private_target(sortlab-example-basic)

if(BUILD_TESTING)
  add_test(NAME sortlab.example.basic COMMAND sortlab-example-basic)
endif()
