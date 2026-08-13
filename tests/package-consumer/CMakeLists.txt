cmake_minimum_required(VERSION 3.23)
project(sortlab_package_consumer LANGUAGES CXX)

find_package(sortlab 1 CONFIG REQUIRED)

add_executable(sortlab-package-consumer main.cpp)
target_link_libraries(sortlab-package-consumer PRIVATE sortlab::sortlab)
target_compile_features(sortlab-package-consumer PRIVATE cxx_std_23)
set_target_properties(sortlab-package-consumer PROPERTIES CXX_EXTENSIONS OFF)
