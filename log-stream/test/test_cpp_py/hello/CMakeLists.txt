cmake_minimum_required (VERSION 2.6)
project (Hello)
# The version number.
set (Tutorial_VERSION_MAJOR 1)
set (Tutorial_VERSION_MINOR 0)
 
# configure a header file to pass some of the CMake settings
# to the source code
 
# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories("/usr/include/python3.8")

# add the executable
add_executable(hello helloworld.cpp)
target_link_libraries (hello python3.8)
