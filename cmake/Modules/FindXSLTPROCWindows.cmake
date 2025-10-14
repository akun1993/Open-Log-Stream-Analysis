if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_LIB_SUFFIX 64)
else()
  set(_LIB_SUFFIX 32)
endif()


find_program(
  XSLTPROC_EXECUTABLE
  NAMES xsltproc
  HINTS ENV XSLTPROC_PATH ${XSLTPROC_PATH} ${CMAKE_SOURCE_DIR}/${XSLTPROC_PATH}
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

get_filename_component(XSLTPROC_DIR ${XSLTPROC_EXECUTABLE} DIRECTORY)

if (${XSLTPROC_EXECUTABLE} STREQUAL "XSLTPROC_EXECUTABLE-NOTFOUND")
    message (FATAL_ERROR "required xsltproc program but not found!")
else()
    message (STATUS "xsltproc program  found in ${XSLTPROC_EXECUTABLE}")
endif()

#find_package(XSLTPROC QUIET 2)
