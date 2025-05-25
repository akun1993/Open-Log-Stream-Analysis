if(POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

if(POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

project(olspython)

if(OS_MACOS)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_package(SWIG 4 REQUIRED)
elseif(OS_POSIX)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  find_package(SWIG 3 REQUIRED)
elseif(OS_WINDOWS)
  find_package(PythonWindows REQUIRED)
  find_package(SwigWindows 3 REQUIRED)
endif()
include(UseSWIG)

set_source_files_properties(
  olspython.i
  PROPERTIES
    USE_TARGET_INCLUDE_DIRECTORIES TRUE
    SWIG_FLAGS
    "$<IF:$<AND:$<BOOL:${OS_POSIX}>,$<NOT:$<BOOL:${OS_MACOS}>>>,-py3,-py3-stable-abi>"
)

swig_add_library(
  olspython
  LANGUAGE python
  TYPE MODULE
  SOURCES olspython.i ../cstrcache.cpp ../cstrcache.h)

target_link_libraries(olspython PRIVATE OLS::scripting OLS::libols)

list(APPEND _SWIG_DEFINITIONS "SWIG_TYPE_TABLE=olspython" "Py_ENABLE_SHARED=1"
     "SWIG_PYTHON_INTERPRETER_NO_DEBUG")

target_compile_features(olspython PRIVATE cxx_auto_type c_std_11)

target_compile_definitions(
  olspython PRIVATE SWIG_TYPE_TABLE=olspython Py_ENABLE_SHARED=1
                    SWIG_PYTHON_INTERPRETER_NO_DEBUG)


set_target_properties(olspython PROPERTIES SWIG_COMPILE_DEFINITIONS
                                           "${_SWIG_DEFINITIONS}")

if(OS_WINDOWS)
  set_target_properties(
    olspython PROPERTIES SWIG_COMPILE_DEFINITIONS
                         "${_SWIG_DEFINITIONS};MS_NO_COREDLL")

  target_link_libraries(olspython PRIVATE Python::Python)

  target_compile_options(olspython PRIVATE /wd4100 /wd4197)

  if(MSVC)
    add_custom_command(
      TARGET olspython
      POST_BUILD
      COMMAND
        "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/olspython.py"
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/olspython.py"
      VERBATIM)
  endif()

elseif(OS_MACOS)
  get_target_property(_PYTHON_INCLUDE_DIRECTORY Python::Python
                      INTERFACE_INCLUDE_DIRECTORIES)

  target_include_directories(olspython PRIVATE ${_PYTHON_INCLUDE_DIRECTORY})

  target_link_options(olspython PRIVATE -undefined dynamic_lookup)

  target_compile_options(olspython PRIVATE -Wno-unused-parameter)

  if(XCODE)
    add_custom_command(
      TARGET olspython
      POST_BUILD
      COMMAND
        "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/olspython.py"
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/olspython.py"
      VERBATIM)
  endif()

  set_target_properties(olspython PROPERTIES MACHO_CURRENT_VERSION 0
                                             MACHO_COMPATIBILITY_VERSION 0)
elseif(OS_POSIX)
  target_link_libraries(olspython PRIVATE Python::Python)

  target_compile_options(olspython PRIVATE -Wno-unused-parameter)

endif()

set_target_properties(
  olspython
  PROPERTIES FOLDER "scripting"
             VERSION "${OLS_VERSION_MAJOR}"
             SOVERSION "${OLS_VERSION_CANONICAL}")

setup_script_plugin_target(olspython)
