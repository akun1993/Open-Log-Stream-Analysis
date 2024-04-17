if(POLICY CMP0090)
  cmake_policy(SET CMP0090 NEW)
endif()

project(libols)

# cmake-format: off
add_library(libols-version STATIC EXCLUDE_FROM_ALL)
add_library(OLS::libols-version ALIAS libols-version)
# cmake-format: on
configure_file(olsversion.c.in olsversion.c @ONLY)
target_sources(libols-version PRIVATE olsversion.c olsversion.h)
target_include_directories(libols-version PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET libols-version PROPERTY FOLDER core)

find_package(Jansson 2.5 REQUIRED)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_library(libols SHARED)
add_library(OLS::libols ALIAS libols)

target_sources(
  libols
  PRIVATE ols.c
          ols.h
          ols.hpp
          ols-data.c
          ols-data.h
          ols-defs.h
          ols-hotkey.c
          ols-hotkey.h
          ols-hotkeys.h
          ols-hotkey-name-map.c
          ols-interaction.h
          ols-internal.h
          ols-module.c
          ols-module.h
          ols-output.c
          ols-output.h
          ols-properties.c
          ols-properties.h
          ols-source.c
          ols-source.h
          ols-config.h)


target_sources(
  libols
  PRIVATE callback/calldata.c
          callback/calldata.h
          callback/decl.c
          callback/decl.h
          callback/signal.c
          callback/signal.h
          callback/proc.c
          callback/proc.h)


target_sources(
  libols
  PRIVATE util/array-serializer.c
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
          util/deque.h
          util/dstr.c
          util/dstr.h
          util/file-serializer.c
          util/file-serializer.h
          util/lexer.c
          util/lexer.h
          util/platform.c
          util/platform.h
          util/profiler.c
          util/profiler.h
          util/profiler.hpp
          util/pipe.h
          util/serializer.h
          util/sse-intrin.h
          util/task.c
          util/task.h
          util/text-lookup.c
          util/text-lookup.h
          util/threading.h
          util/utf8.c
          util/utf8.h
          util/uthash.h
          util/util_uint64.h
          util/util_uint128.h
          util/curl/curl-helper.h
          util/darray.h
          util/util.hpp)




target_link_libraries(
  libols
  PRIVATE Jansson::Jansson
          OLS::uthash
          OLS::libols-version
          ZLIB::ZLIB
  PUBLIC Threads::Threads)

set_target_properties(
  libols
  PROPERTIES OUTPUT_NAME ols
             FOLDER "core"
             VERSION "${OLS_VERSION_MAJOR}"
             SOVERSION "0")

target_compile_definitions(
  libols
  PUBLIC ${ARCH_SIMD_DEFINES}
  PRIVATE IS_LIBOLS)

target_compile_features(libols PRIVATE cxx_alias_templates)

target_compile_options(libols PUBLIC ${ARCH_SIMD_FLAGS})

target_include_directories(libols PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                         $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/config>)

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OLS Library")
  set(UI_VERSION "${OLS_VERSION_CANONICAL}")

  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in libols.rc)

  target_sources(
    libols
    PRIVATE ols-win-crash-handler.c
            ols-windows.c
            util/threading-windows.c
            util/threading-windows.h
            util/pipe-windows.c
            util/platform-windows.c
            util/windows/device-enum.c
            util/windows/device-enum.h
            util/windows/obfuscate.c
            util/windows/obfuscate.h
            util/windows/win-registry.h
            util/windows/win-version.h
            util/windows/window-helpers.c
            util/windows/window-helpers.h
            util/windows/ComPtr.hpp
            util/windows/CoTaskMemPtr.hpp
            util/windows/HRError.hpp
            util/windows/WinHandle.hpp
            libols.rc)

  target_compile_definitions(libols PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
  set_source_files_properties(ols-win-crash-handler.c PROPERTIES COMPILE_DEFINITIONS
                                                                 OLS_VERSION="${OLS_VERSION_CANONICAL}")
  target_link_libraries(libols PRIVATE dxgi Avrt Dwmapi winmm Rpcrt4)

  if(MSVC)
    target_link_libraries(libols PUBLIC OLS::w32-pthreads)

    target_compile_options(libols PRIVATE "$<$<COMPILE_LANGUAGE:C>:/EHc->" "$<$<COMPILE_LANGUAGE:CXX>:/EHc->")

    target_link_options(libols PRIVATE "LINKER:/IGNORE:4098" "LINKER:/SAFESEH:NO")

    add_library(ols-comutils INTERFACE)
    add_library(OLS::COMutils ALIAS ols-comutils)
    target_sources(ols-comutils INTERFACE util/windows/ComPtr.hpp)
    target_include_directories(ols-comutils INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

    add_library(ols-winhandle INTERFACE)
    add_library(OLS::winhandle ALIAS ols-winhandle)
    target_sources(ols-winhandle INTERFACE util/windows/WinHandle.hpp)
    target_include_directories(ols-winhandle INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

elseif(OS_MACOS)

  find_library(COCOA Cocoa)
  find_library(APPKIT AppKit)
  find_library(IOKIT IOKit)
  find_library(CARBON Carbon)

  mark_as_advanced(
    COCOA
    APPKIT
    IOKIT
    CARBON)

  target_link_libraries(
    libols
    PRIVATE ${COCOA}
            ${APPKIT}
            ${IOKIT}
            ${CARBON})

  target_sources(
    libols
    PRIVATE ols-cocoa.m
            util/pipe-posix.c
            util/platform-cocoa.m
            util/platform-nix.c
            util/threading-posix.c
            util/threading-posix.h
            util/apple/cfstring-utils.h)

  set_source_files_properties(util/platform-cocoa.m ols-cocoa.m PROPERTIES COMPILE_FLAGS -fobjc-arc)

  set_target_properties(libols PROPERTIES SOVERSION "1" BUILD_RPATH "$<TARGET_FILE_DIR:OLS::libols-opengl>")

elseif(OS_POSIX)
  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    target_compile_definitions(libols PRIVATE ENABLE_DARRAY_TYPE_TEST)
  endif()

  find_package(LibUUID REQUIRED)

  target_sources(
    libols
    PRIVATE util/threading-posix.c
            util/threading-posix.h
            util/pipe-posix.c
            util/platform-nix.c)

  target_link_libraries(libols PRIVATE LibUUID::LibUUID)



  if(OS_FREEBSD)
    find_package(Sysinfo REQUIRED)
    target_link_libraries(libols PRIVATE Sysinfo::Sysinfo)
  endif()

  set_target_properties(libols PROPERTIES BUILD_RPATH "$<TARGET_FILE_DIR:OLS::libols-opengl>")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/olsconfig.h.in ${CMAKE_BINARY_DIR}/config/olsconfig.h)

target_compile_definitions(
  libols
  PUBLIC HAVE_OLSCONFIG_H
  PRIVATE "OLS_INSTALL_PREFIX=\"${OLS_INSTALL_PREFIX}\"" "$<$<BOOL:${LINUX_PORTABLE}>:LINUX_PORTABLE>")

if(ENABLE_FFMPEG_MUX_DEBUG)
  target_compile_definitions(libols PRIVATE SHOW_SUBPROCESSES)
endif()

get_target_property(_OLS_SOURCES libols SOURCES)
set(_OLS_HEADERS ${_OLS_SOURCES})
set(_OLS_FILTERS ${_OLS_SOURCES})
list(FILTER _OLS_HEADERS INCLUDE REGEX ".*\\.h(pp)?")
list(FILTER _OLS_SOURCES INCLUDE REGEX ".*\\.(m|c[cp]?p?)")
list(FILTER _OLS_FILTERS INCLUDE REGEX ".*\\.effect")

source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  PREFIX "Source Files"
  FILES ${_OLS_SOURCES})
source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  PREFIX "Header Files"
  FILES ${_OLS_HEADERS})
source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}"
  PREFIX "Effect Files"
  FILES ${_OLS_FILTERS})

setup_binary_target(libols)
setup_target_resources(libols libols)
export_target(libols)
install_headers(libols)
