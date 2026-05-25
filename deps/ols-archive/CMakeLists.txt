project(ols-archive)

ols_status(STATUS "Configuring ols-archive...")

add_library(ols-archive SHARED)
add_library(OLS::archive ALIAS ols-archive)

target_sources(
  ols-archive
  PUBLIC ols-archive.h
  PRIVATE ols-archive.cpp)

target_link_libraries(ols-archive PRIVATE OLS::libols)

target_compile_features(ols-archive PRIVATE cxx_std_17)

target_include_directories(ols-archive PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                              ${CMAKE_BINARY_DIR}/config)

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OLS Studio archive module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
                 ols-archive.rc)

  target_sources(ols-archive PRIVATE ols-archive.rc)

elseif(OS_MACOS)
  target_link_libraries(ols-archive PRIVATE objc)
endif()

set_target_properties(
  ols-archive
  PROPERTIES FOLDER "deps"
             VERSION "${OLS_VERSION_MAJOR}"
             SOVERSION "1")

setup_binary_target(ols-archive)
