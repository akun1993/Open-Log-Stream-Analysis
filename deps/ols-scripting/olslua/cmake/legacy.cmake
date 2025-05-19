if(POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

if(POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

project(olslua)

find_package(Luajit REQUIRED)

if(OS_MACOS)
  find_package(SWIG 4 REQUIRED)
elseif(OS_POSIX)
  find_package(SWIG 3 REQUIRED)
elseif(OS_WINDOWS)
  find_package(SwigWindows 3 REQUIRED)
endif()
include(UseSWIG)

set_source_files_properties(olslua.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES TRUE)

swig_add_library(
  olslua
  LANGUAGE lua
  TYPE MODULE
  SOURCES olslua.i ../cstrcache.cpp ../cstrcache.h)

target_link_libraries(olslua PRIVATE ols::scripting ols::libols Luajit::Luajit)

list(APPEND _SWIG_DEFINITIONS "SWIG_TYPE_TABLE=olslua" "SWIG_LUA_INTERPRETER_NO_DEBUG")

set_target_properties(
  olslua
  PROPERTIES FOLDER "scripting"
             VERSION "${OLS_VERSION_MAJOR}"
             SOVERSION "${OLS_VERSION_CANONICAL}")

target_compile_definitions(olslua PRIVATE SWIG_TYPE_TABLE=olslua SWIG_LUA_INTERPRETER_NO_DEBUG)

if(ENABLE_UI)
  #list(APPEND _SWIG_DEFINITIONS "ENABLE_UI")
  #target_link_libraries(olslua PRIVATE OBS::frontend-api)

  #target_compile_definitions(olslua PRIVATE ENABLE_UI)
endif()

set_target_properties(olslua PROPERTIES SWIG_COMPILE_DEFINITIONS "${_SWIG_DEFINITIONS}")

if(OS_WINDOWS)
  if(MSVC)
    target_compile_options(olslua PRIVATE /wd4054 /wd4197 /wd4244 /wd4267)
  endif()
elseif(OS_MACOS)
  set_target_properties(olslua PROPERTIES MACHO_CURRENT_VERSION 0 MACHO_COMPATIBILITY_VERSION 0)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND SWIG_VERSION VERSION_LESS "4.1")
  target_compile_options(olslua PRIVATE -Wno-maybe-uninitialized)
endif()

setup_script_plugin_target(olslua)
