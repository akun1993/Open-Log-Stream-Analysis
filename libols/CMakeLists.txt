if(POLICY CMP0090)
  cmake_policy(SET CMP0090 NEW)
endif()

project(libols)

find_package(Threads REQUIRED)

# cmake-format: off

# cmake-format: on

find_package(ZLIB REQUIRED)


if(ENABLE_UI)
  find_qt(COMPONENTS Core)
else()
  set(_QT_VERSION 0)
endif()

if(NOT TARGET OLS::uthash)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/uthash" "${CMAKE_BINARY_DIR}/deps/uthash")
endif()

add_library(libols SHARED)
add_library(OLS::libols ALIAS libols)

target_sources(
  libols
  PRIVATE # cmake-format: sortable
          ols-data.c
          ols-data.h
          ols-defs.h
          ols-internal.h
          ols-module.c
          ols-module.h
          ols-output.c
          ols-output.h
          ols-properties.c
          ols-properties.h
          ols-source.c
          ols-source.h
          ols-process.c
          ols-process.h
          ols.c
          ols.h
          ols-task.c
          ols-task.h          
          ols-caps.c
          ols-caps.h
          ols-pad.c
          ols-pad.h
          ols-event.c
          ols-event.h
          ols-mini-object.c
          ols-mini-object.h
          ols-buffer.c
          ols-buffer.h
          ols-buffer-list.c
          ols-buffer-list.h
          ols-meta.h
          ols-meta-txt.h
          ols-meta-txt.c
          ols-meta-result.h
          ols-meta-result.c          
          ols.hpp)

target_sources(
  libols
  PRIVATE # cmake-format: sortable
          util/array-serializer.c
          util/array-serializer.h
          util/base.c
          util/base.h
          util/bitstream.c
          util/bitstream.h
          util/bmem.c
          util/bmem.h
          util/c99defs.h
          util/cf-lexer.c
          util/cf-lexer.h
          util/cf-parser.c
          util/cf-parser.h
          util/circlebuf.h
          util/config-file.c
          util/config-file.h
          util/crc32.c
          util/crc32.h
          util/darray.h
          util/deque.h
          util/dstr.c
          util/dstr.h
          util/file-serializer.c
          util/file-serializer.h
          util/lexer.c
          util/lexer.h
          util/pipe.h
          util/platform.c
          util/platform.h
          util/profiler.c
          util/profiler.h
          util/profiler.hpp
          util/serializer.h
          util/sse-intrin.h
          util/task.c
          util/task.h
          util/text-lookup.c
          util/text-lookup.h
          util/threading.h
          util/utf8.c
          util/utf8.h
          util/time-parse.c
          util/time-parse.h   
          util/str-util.c
          util/str-util.h                     
          util/uthash.h
          util/util.hpp
          util/util_uint128.h
          util/util_uint64.h)

target_sources(
  libols
  PRIVATE # cmake-format: sortable
          callback/calldata.c
          callback/calldata.h
          callback/decl.c
          callback/decl.h
          callback/proc.c
          callback/proc.h
          callback/signal.c
          callback/signal.h)


target_compile_features(libols PUBLIC cxx_std_17)

target_compile_definitions(
  libols
  PRIVATE IS_LIBOLS)

target_link_libraries(
  libols
  PRIVATE Jansson::Jansson
          OLS::uthash
          ZLIB::ZLIB
  PUBLIC Threads::Threads)

set_target_properties(
  libols
  PROPERTIES OUTPUT_NAME ols
              FOLDER "core"
              VERSION "${OLS_VERSION_MAJOR}"
              SOVERSION "0")

target_compile_features(libols PRIVATE cxx_alias_templates)
#message("dir is ${uthash__INCLUDE_DIRS}")
target_include_directories(
  libols PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/config> 
                $<BUILD_INTERFACE:${uthash__INCLUDE_DIRS}> 
                $<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX)
  include(cmake/os-linux.cmake)
elseif(OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-freebsd.cmake)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/olsconfig.h.in "${CMAKE_BINARY_DIR}/config/olsconfig.h" @ONLY)

target_include_directories(libols PUBLIC "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/config>"
                                         "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                                         "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../deps/libz/include"  )
#message("dir is ${CMAKE_BINARY_DIR}")
#message("dir is ${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(libols PUBLIC HAVE_OLSCONFIG_H)

set(public_headers
    # cmake-format: sortable
    callback/calldata.h
    callback/decl.h
    callback/proc.h
    callback/signal.h
    ols-config.h
    ols-data.h
    ols-defs.h
    ols-interaction.h
    ols-module.h
    ols-nix-platform.h
    ols-output.h
    ols-properties.h
    ols-source.h
    ols-process.h
    ols.h
    ols.hpp
    util/apple/cfstring-utils.h
    util/array-serializer.h
    util/base.h
    util/bitstream.h
    util/bmem.h
    util/c99defs.h
    util/cf-lexer.h
    util/cf-parser.h
    util/circlebuf.h
    util/config-file.h
    util/crc32.h
    util/darray.h
    util/deque.h
    util/dstr.h
    util/dstr.hpp
    util/file-serializer.h
    util/lexer.h
    util/pipe.h
    util/platform.h
    util/profiler.h
    util/profiler.hpp
    util/serializer.h
    util/task.h
    util/text-lookup.h
    util/threading-posix.h
    util/threading-windows.h
    util/threading.h
    util/uthash.h
    util/util.hpp
    util/util_uint128.h
    util/util_uint64.h
    util/windows/ComPtr.hpp
    util/windows/CoTaskMemPtr.hpp
    util/windows/device-enum.h
    util/windows/HRError.hpp
    util/windows/win-registry.h
    util/windows/win-version.h
    util/windows/window-helpers.h
    util/windows/WinHandle.hpp)

# cmake-format: off
set_property(TARGET libols APPEND PROPERTY OLS_PUBLIC_HEADERS ${public_headers})

get_target_property(_OLS_SOURCES libols SOURCES)
set(_OLS_HEADERS ${_OLS_SOURCES})

list(FILTER _OLS_HEADERS INCLUDE REGEX ".*\\.h(pp)?")
list(FILTER _OLS_SOURCES INCLUDE REGEX ".*\\.(m|c[cp]?p?)")

source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  PREFIX "Source Files"
  FILES ${_OLS_SOURCES})
source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  PREFIX "Header Files"
  FILES ${_OLS_HEADERS})


setup_binary_target(libols)
setup_target_resources(libols libols)
export_target(libols)
install_headers(libols)