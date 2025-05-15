cmake_minimum_required(VERSION 3.16...3.21)

legacy_check()

if(NOT ENABLE_SCRIPTING)
  target_disable_feature(ols-scripting "Scripting support")
  return()
endif()

add_library(ols-cstrcache INTERFACE)
add_library(OLS::cstrcache ALIAS ols-cstrcache)

target_sources(ols-cstrcache INTERFACE cstrcache.cpp cstrcache.h)
target_include_directories(ols-cstrcache INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(SWIG 4 REQUIRED)

add_library(ols-scripting SHARED)
add_library(OLS::scripting ALIAS ols-scripting)

include(cmake/lua.cmake)
include(cmake/python.cmake)

if(NOT ENABLE_SCRIPTING_LUA AND NOT ENABLE_SCRIPTING_PYTHON)
  target_disable_feature(ols-scripting "Scripting support")
  return()
else()
  target_enable_feature(ols-scripting "Scripting support")
endif()

target_sources(
  ols-scripting
  PUBLIC obs-scripting.h
  PRIVATE obs-scripting.c obs-scripting-logging.c obs-scripting-callback.h
          $<$<BOOL:${ENABLE_UI}>:obs-scripting-python-frontend.c> $<$<BOOL:${ENABLE_UI}>:obs-scripting-lua-frontend.c>)

target_compile_definitions(ols-scripting PRIVATE SCRIPT_DIR="${OLS_SCRIPT_PLUGIN_PATH}"
                                                 $<$<BOOL:${ENABLE_UI}>:ENABLE_UI>)
target_compile_features(ols-scripting PRIVATE cxx_auto_type)
target_include_directories(ols-scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ols-scripting PRIVATE OBS::libobs OBS::cstrcache $<$<BOOL:${ENABLE_UI}>:OBS::frontend-api>
                                            $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads> $<$<PLATFORM_ID:Darwin>:objc>)

if(OS_WINDOWS)
  configure_file(cmake/windows/ols-module.rc.in ols-scripting.rc)
  target_sources(ols-scripting PRIVATE ols-scripting.rc)
endif()

set_target_properties_ols(ols-scripting PROPERTIES FOLDER scripting PREFIX "")
