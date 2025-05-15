cmake_minimum_required(VERSION 3.16...3.21)

legacy_check()

if(POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

if(POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

find_package(Luajit REQUIRED)

find_package(SWIG 4 REQUIRED)

include(UseSWIG)

set_source_files_properties(olslua.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES TRUE)
swig_add_library(
  olslua
  LANGUAGE lua
  TYPE MODULE
  SOURCES olslua.i)

add_library(ols::lua ALIAS olslua)

target_compile_options(olslua PRIVATE "$<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-error=shorten-64-to-32>")
target_compile_definitions(olslua PRIVATE SWIG_TYPE_TABLE=olslua SWIG_LUA_INTERPRETER_NO_DEBUG
                                          $<$<BOOL:${ENABLE_UI}>:ENABLE_UI>)

target_link_libraries(olslua PRIVATE OLS::scripting OLS::cstrcache OLS::libols Luajit::Luajit
                                     $<$<BOOL:ENABLE_UI>:OLS::frontend-api>)

if(OS_WINDOWS)
  target_compile_options(olslua PRIVATE /wd4054 /wd4197 /wd4244 /wd4267)
elseif(OS_MACOS)
  set_property(TARGET olslua PROPERTY XCODE_ATTRIBUTE_STRIP_STYLE non-global)
  target_compile_options(olslua PRIVATE -Wno-unreachable-code)
elseif(OS_LINUX OR OS_FREEBSD)
  target_compile_options(olslua PRIVATE -Wno-maybe-uninitialized)
endif()

set_property(
  TARGET olslua
  APPEND
  PROPERTY SWIG_COMPILE_DEFINITIONS "SWIG_TYPE_TABLE=olslua" "SWIG_LUA_INTERPRETER_NO_DEBUG"
           "$<$<BOOL:${ENABLE_UI}>:ENABLE_UI>")

set_target_properties_ols(olslua PROPERTIES FOLDER scripting)
