if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_LIB_SUFFIX 64)
else()
  set(_LIB_SUFFIX 32)
endif()

find_program(
  7Z_EXECUTABLE
  NAMES 7z
  HINTS ENV 7Z_PATH ${7Z_PATH} ${CMAKE_SOURCE_DIR}/${7Z_PATH}
	PATH_SUFFIXES
    lib${_lib_suffix}
    lib
    libs${_lib_suffix}
    libs
    bin${_lib_suffix}
    bin
    ../lib${_lib_suffix}
    ../lib
    ../libs${_lib_suffix}
    ../libs
    ../bin${_lib_suffix}
    ../bin)

if (${7Z_EXECUTABLE} STREQUAL "7Z_EXECUTABLE-NOTFOUND")
    message (FATAL_ERROR "required 7z program but not found!")
else()
    message (STATUS "7z program  found in ${7Z_EXECUTABLE}")
endif()

find_package(7Z QUIET 2)
