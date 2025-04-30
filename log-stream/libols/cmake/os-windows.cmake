if(NOT TARGET OLS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

configure_file(cmake/windows/ols-module.rc.in libols.rc)

add_library(ols-obfuscate INTERFACE)
add_library(OLS::obfuscate ALIAS ols-obfuscate)
target_sources(ols-obfuscate INTERFACE util/windows/obfuscate.c util/windows/obfuscate.h)
target_include_directories(ols-obfuscate INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(ols-comutils INTERFACE)
add_library(OLS::COMutils ALIAS ols-comutils)
target_sources(ols-comutils INTERFACE util/windows/ComPtr.hpp)
target_include_directories(ols-comutils INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(ols-winhandle INTERFACE)
add_library(OLS::winhandle ALIAS ols-winhandle)
target_sources(ols-winhandle INTERFACE util/windows/WinHandle.hpp)
target_include_directories(ols-winhandle INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(
  libols
  PRIVATE # cmake-format: sortable
          audio-monitoring/win32/wasapi-enum-devices.c
          audio-monitoring/win32/wasapi-monitoring-available.c
          audio-monitoring/win32/wasapi-output.c
          audio-monitoring/win32/wasapi-output.h
          libols.rc
          ols-win-crash-handler.c
          ols-windows.c
          util/pipe-windows.c
          util/platform-windows.c
          util/threading-windows.c
          util/threading-windows.h
          util/windows/CoTaskMemPtr.hpp
          util/windows/device-enum.c
          util/windows/device-enum.h
          util/windows/HRError.hpp
          util/windows/obfuscate.c
          util/windows/obfuscate.h
          util/windows/win-registry.h
          util/windows/win-version.h
          util/windows/window-helpers.c
          util/windows/window-helpers.h)

target_compile_options(libols PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/EHc->)

set_source_files_properties(ols-win-crash-handler.c PROPERTIES COMPILE_DEFINITIONS
                                                               OLS_VERSION="${OLS_VERSION_CANONICAL}")

target_link_libraries(
  libols
  PRIVATE Avrt
          Dwmapi
          Dxgi
          winmm
          Rpcrt4
          OLS::obfuscate
          OLS::winhandle
          OLS::COMutils
  PUBLIC OLS::w32-pthreads)

target_link_options(libols PRIVATE /IGNORE:4098 /SAFESEH:NO)

set_target_properties(libols PROPERTIES PREFIX "" OUTPUT_NAME "ols")
