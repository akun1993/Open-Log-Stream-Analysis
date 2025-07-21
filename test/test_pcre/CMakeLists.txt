cmake_minimum_required (VERSION 2.6)
project (test_pcre)
# The version number.
set (Tutorial_VERSION_MAJOR 1)
set (Tutorial_VERSION_MINOR 0)
 
# configure a header file to pass some of the CMake settings
# to the source code
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../build/deps/pcre2/interface)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../build/deps/pcre2)

# add the executable
add_executable(test_pcre pcre2demo.c)
target_link_libraries (test_pcre pcre2-8)


