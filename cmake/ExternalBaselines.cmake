option(SORTLAB_ENABLE_EXTERNAL_BASELINES "Build pinned third-party sorting comparison track" OFF)

if(SORTLAB_ENABLE_EXTERNAL_BASELINES)
  set(SORTLAB_PDQ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/vendor/pdqsort")
  set(SORTLAB_IPS4O_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/vendor/ips4o")
  set(SORTLAB_PDQ_COMMIT "b1ef26a55cdb60d236a5cb199c4234c704f46726")
  set(SORTLAB_IPS4O_COMMIT "08a5b926ee65cef19139057c6bde02bb5542c1cb")

  foreach(required
      "${SORTLAB_PDQ_DIR}/pdqsort.h"
      "${SORTLAB_PDQ_DIR}/license.txt"
      "${SORTLAB_IPS4O_DIR}/include/ips4o.hpp"
      "${SORTLAB_IPS4O_DIR}/LICENSE")
    if(NOT EXISTS "${required}")
      message(FATAL_ERROR "Missing pinned external baseline file: ${required}. Run tools/bootstrap_external.py first.")
    endif()
  endforeach()

  sortlab_executable(sort_external src/sort_external.cpp)
  target_include_directories(sort_external PRIVATE
    "${SORTLAB_PDQ_DIR}"
    "${SORTLAB_IPS4O_DIR}/include")
  target_compile_definitions(sort_external PRIVATE
    SORTLAB_PDQ_COMMIT="${SORTLAB_PDQ_COMMIT}"
    SORTLAB_IPS4O_COMMIT="${SORTLAB_IPS4O_COMMIT}")

  if(BUILD_TESTING)
    add_test(NAME external_sort_self_test COMMAND sort_external --self-test)
  endif()
endif()
