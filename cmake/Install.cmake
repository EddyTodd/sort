install(TARGETS sortlab
  EXPORT sortlabTargets
  FILE_SET public_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT sortlabTargets
  FILE sortlabTargets.cmake
  NAMESPACE sortlab::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortlab)

configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/sortlabConfig.cmake.in
  ${PROJECT_BINARY_DIR}/sortlabConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortlab)

write_basic_package_version_file(
  ${PROJECT_BINARY_DIR}/sortlabConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES
  ${PROJECT_BINARY_DIR}/sortlabConfig.cmake
  ${PROJECT_BINARY_DIR}/sortlabConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortlab)

install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/sortlab)
