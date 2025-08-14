set(MODULE_DESCRIPTION "OLS Library")
set(UI_VERSION "${OLS_VERSION_CANONICAL}")


if(NOT TARGET OLS::obfuscate)
  add_library(ols-obfuscate INTERFACE)
  add_library(OLS::obfuscate ALIAS ols-obfuscate)
  target_sources(ols-obfuscate INTERFACE util/windows/obfuscate.c util/windows/obfuscate.h)
  target_include_directories(ols-obfuscate INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT TARGET OLS::comutils)
  add_library(ols-comutils INTERFACE)
  add_library(OLS::COMutils ALIAS ols-comutils)
  target_sources(ols-comutils INTERFACE util/windows/ComPtr.hpp)
  target_include_directories(ols-comutils INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT TARGET OLS::winhandle)
  add_library(ols-winhandle INTERFACE)
  add_library(OLS::winhandle ALIAS ols-winhandle)
  target_sources(ols-winhandle INTERFACE util/windows/WinHandle.hpp)
  target_include_directories(ols-winhandle INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT TARGET OLS::threading-windows)
  add_library(ols-threading-windows INTERFACE)
  add_library(OLS::threading-windows ALIAS ols-threading-windows)
  target_sources(ols-threading-windows INTERFACE util/threading-windows.h)
  target_include_directories(ols-threading-windows INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(NOT TARGET OLS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

# if(NOT OLS_PARENT_ARCHITECTURE STREQUAL CMAKE_VS_PLATFORM_NAME)
#   return()
# endif()


configure_file(cmake/windows/ols-module.rc.in
               libols.rc)

target_sources(
  libols
  PRIVATE ols-win-crash-handler.c
          ols-windows.c
          util/threading-windows.c
          util/threading-windows.h
          util/pipe-windows.c
          util/platform-windows.c
          util/windows/obfuscate.c
          util/windows/obfuscate.h
          util/windows/win-version.h
          util/windows/window-helpers.c
          util/windows/window-helpers.h
          util/windows/ComPtr.hpp
          util/windows/CoTaskMemPtr.hpp
          util/windows/HRError.hpp
          util/windows/WinHandle.hpp
          libols.rc)

target_compile_definitions(
  libols PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS
                 _CRT_NONSTDC_NO_WARNINGS)

#target_link_libraries(libols PRIVATE  )

if(MSVC)
  target_link_libraries(libols  PRIVATE  Dwmapi  winmm Rpcrt4 OLS::obfuscate OLS::winhandle OLS::COMutils PUBLIC OLS::w32-pthreads)

  target_compile_options(libols PRIVATE "$<$<COMPILE_LANGUAGE:C>:/EHc->"
                                        "$<$<COMPILE_LANGUAGE:CXX>:/EHc->")

  target_link_options(libols PRIVATE "LINKER:/IGNORE:4098"
                      "LINKER:/SAFESEH:NO")
endif()