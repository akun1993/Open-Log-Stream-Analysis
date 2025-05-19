cmake_minimum_required(VERSION 3.16...3.21)

legacy_check()

if(POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

if(POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

if(POLICY CMP0094)
  cmake_policy(SET CMP0094 NEW)
endif()

find_package(Python 3.8...<3.11 REQUIRED Interpreter Development)
find_package(SWIG 4 REQUIRED)

include(UseSWIG)

set_source_files_properties(olspython.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES TRUE
                                                   SWIG_FLAGS "$<IF:$<BOOL:${OS_LINUX}>,-py3,-py3-stable-abi>")

swig_add_library(
  olspython
  LANGUAGE python
  TYPE MODULE
  SOURCES olspython.i)

add_library(OLS::python ALIAS olspython)

file(
  GENERATE
  OUTPUT $<$<PLATFORM_ID:Windows>:$<CONFIG>/>olspython.h
  CONTENT "#pragma once\n\n#define PYTHON_LIB \"$<TARGET_LINKER_FILE_NAME:Python::Python>\"\n")

target_include_directories(olspython PRIVATE "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>>")

list(APPEND _SWIG_DEFINITIONS "SWIG_TYPE_TABLE=olspython" "Py_ENABLE_SHARED=1" "SWIG_PYTHON_INTERPRETER_NO_DEBUG")

target_compile_definitions(olspython PRIVATE SWIG_TYPE_TABLE=olspython Py_ENABLE_SHARED=1
                                             SWIG_PYTHON_INTERPRETER_NO_DEBUG $<$<BOOL:${ENABLE_UI}>:ENABLE_UI>)

target_link_libraries(olspython PRIVATE OLS::scripting OLS::cstrcache OLS::libols)

if(OS_WINDOWS)
  target_link_libraries(olspython PRIVATE Python::Python)
  target_compile_options(olspython PRIVATE /wd4100 /wd4197)
elseif(OS_MACOS)
  get_target_property(_python_include_directory Python::Python INTERFACE_INCLUDE_DIRECTORIES)
  target_include_directories(olspython PRIVATE ${_python_include_directory})
  target_compile_options(olspython PRIVATE -Wno-unused-parameter -Wno-macro-redefined -Wno-unreachable-code)
  target_link_options(olspython PUBLIC LINKER:-undefined,dynamic_lookup)

  set_property(TARGET olspython PROPERTY XCODE_ATTRIBUTE_STRIP_STYLE non-global)
elseif(OS_LINUX OR OS_FREEBSD)
  target_link_libraries(olspython PRIVATE Python::Python)
endif()

if(MSVC OR XCODE)
  add_custom_command(
    TARGET olspython
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/olspython.py"
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/olspython.py"
    VERBATIM)
endif()

set_property(
  TARGET olspython
  APPEND
  PROPERTY SWIG_COMPILE_DEFINITIONS "SWIG_TYPE_TABLE=olspython" "Py_ENABLE_SHARED=1" "SWIG_PYTHON_INTERPRETER_NO_DEBUG"
           "$<$<BOOL:${ENABLE_UI}>:ENABLE_UI>" "$<$<PLATFORM_ID:Windows>:MS_NO_COREDLL>")

set_target_properties_ols(olspython PROPERTIES FOLDER scripting PREFIX "_")
