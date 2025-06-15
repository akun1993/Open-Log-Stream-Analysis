cmake_minimum_required (VERSION 2.6)
project (test_str_list)
# The version number.
set (Tutorial_VERSION_MAJOR 1)
set (Tutorial_VERSION_MINOR 0)
 
# configure a header file to pass some of the CMake settings
# to the source code
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("/usr/include/python3.8")

# add the executable
add_executable(test_str_list test_str_list.c)
target_link_libraries (test_str_list python3.8)
