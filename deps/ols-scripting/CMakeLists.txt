cmake_minimum_required(VERSION 3.16...3.21)


option(ENABLE_SCRIPTING_LUA "Enable Lua scripting support" ON)
option(ENABLE_SCRIPTING_PYTHON "Enable Python scripting support" ON)

if(NOT ENABLE_SCRIPTING)
  ols_status(DISABLED "ols-scripting")
  return()
endif()

project(ols-scripting)

if(ENABLE_SCRIPTING_LUA)
  add_subdirectory(olslua)
  find_package(Luajit)

  if(NOT TARGET Luajit::Luajit)
    ols_status(FATAL_ERROR "ols-scripting -> Luajit not found.")
    return()
  else()
    ols_status(STATUS "ols-scripting -> Luajit found.")
  endif()
else()
  ols_status(DISABLED "Luajit support")
endif()

if(ENABLE_SCRIPTING_PYTHON)
  add_subdirectory(olspython)
  if(OS_WINDOWS)
    find_package(PythonWindows)
  else()
    find_package(Python COMPONENTS Interpreter Development)
  endif()

  if(NOT TARGET Python::Python)
    ols_status(FATAL_ERROR "ols-scripting -> Python not found.")
    return()
  else()
    ols_status(STATUS "ols-scripting -> Python ${Python_VERSION} found.")
  endif()
else()
  ols_status(DISABLED "Python support")
endif()

if(NOT TARGET Luajit::Luajit AND NOT TARGET Python::Python)
  ols_status(WARNING "ols-scripting -> No supported scripting libraries found.")
  return()
endif()

if(OS_MACOS)
  find_package(SWIG 4 REQUIRED)
elseif(OS_POSIX)
  find_package(SWIG 3 REQUIRED)
elseif(OS_WINDOWS)
  find_package(SwigWindows 3 REQUIRED)
endif()

add_library(ols-scripting SHARED)
add_library(OLS::scripting ALIAS ols-scripting)

target_sources(
  ols-scripting
  PUBLIC ols-scripting.h
  PRIVATE ols-scripting.c cstrcache.cpp cstrcache.h ols-scripting-logging.c
          ols-scripting-callback.h)

target_link_libraries(ols-scripting PRIVATE OLS::libols)

target_compile_features(ols-scripting PRIVATE cxx_auto_type)

target_include_directories(ols-scripting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${CMAKE_BINARY_DIR}/config)

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OLS Studio scripting module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
                 ols-scripting.rc)

  target_sources(ols-scripting PRIVATE ols-scripting.rc)

  target_link_libraries(ols-scripting PRIVATE OLS::w32-pthreads)

elseif(OS_MACOS)
  target_link_libraries(ols-scripting PRIVATE objc)
endif()

set_target_properties(
  ols-scripting
  PROPERTIES FOLDER "scripting"
             VERSION "${OLS_VERSION_MAJOR}"
             SOVERSION "1")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/swig)

if(TARGET Luajit::Luajit)
  add_custom_command(
    OUTPUT swig/swigluarun.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E env "SWIG_LIB=${SWIG_DIR}" ${SWIG_EXECUTABLE}
            -lua -external-runtime swig/swigluarun.h
    COMMENT "ols-scripting - generating Luajit SWIG interface headers")

  set_source_files_properties(swig/swigluarun.h PROPERTIES GENERATED ON)

  target_link_libraries(ols-scripting PRIVATE Luajit::Luajit)

  target_sources(
    ols-scripting
    PRIVATE ols-scripting-lua.c ols-scripting-lua.h ols-scripting-lua-source.c
            ${CMAKE_CURRENT_BINARY_DIR}/swig/swigluarun.h)

  target_include_directories(ols-scripting PRIVATE ${CMAKE_CURRENT_BINARY_DIR})



endif()

if(TARGET Python::Python)
  add_custom_command(
    OUTPUT swig/swigpyrun.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PRE_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E env "SWIG_LIB=${SWIG_DIR}" ${SWIG_EXECUTABLE} -python
      $<IF:$<AND:$<BOOL:${OS_POSIX}>,$<NOT:$<BOOL:${OS_MACOS}>>>,-py3,-py3-stable-abi>
      -external-runtime swig/swigpyrun.h
    COMMENT "ols-scripting - generating Python 3 SWIG interface headers")

  set_source_files_properties(swig/swigpyrun.h PROPERTIES GENERATED ON)

  target_sources(
    ols-scripting
    PRIVATE ols-scripting-python.c ols-scripting-python.h
            ols-scripting-python-import.h
            ${CMAKE_CURRENT_BINARY_DIR}/swig/swigpyrun.h)

  target_include_directories(ols-scripting PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

  get_filename_component(_PYTHON_PATH "${Python_LIBRARIES}" PATH)
  get_filename_component(_PYTHON_FILE "${Python_LIBRARIES}" NAME)

  string(REGEX REPLACE "\\.[^.]*$" "" _PYTHON_FILE ${_PYTHON_FILE})

  if(OS_WINDOWS)
    string(REGEX REPLACE "_d" "" _PYTHON_FILE ${_PYTHON_FILE})
  endif()
  set(OLS_SCRIPT_PYTHON_PATH "${_PYTHON_FILE}")

  unset(_PYTHON_FILE)
  unset(_PYTHON_PATH)

  if(OS_WINDOWS OR OS_MACOS)
    target_include_directories(ols-scripting PRIVATE ${Python_INCLUDE_DIRS})

    target_sources(ols-scripting PRIVATE ols-scripting-python-import.c)
    if(OS_MACOS)
      target_link_options(ols-scripting PRIVATE -undefined dynamic_lookup)
    endif()
  else()
    target_link_libraries(ols-scripting PRIVATE Python::Python)
  endif()


endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ols-scripting-config.h.in
               ${CMAKE_BINARY_DIR}/config/ols-scripting-config.h)

target_sources(ols-scripting
               PUBLIC ${CMAKE_BINARY_DIR}/config/ols-scripting-config.h)

setup_binary_target(ols-scripting)

