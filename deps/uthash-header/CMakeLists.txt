cmake_minimum_required(VERSION 3.16...3.25)

add_library(uthash INTERFACE)
add_library(OLS::uthash ALIAS uthash)

target_sources(uthash INTERFACE uthash/uthash.h)

target_include_directories(uthash INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

# Generate the config file for the build-tree.
set(uthash__INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

