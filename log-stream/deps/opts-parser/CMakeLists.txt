project(opts-parser)

add_library(opts-parser INTERFACE)
add_library(OLS::opts-parser ALIAS opts-parser)

target_sources(opts-parser INTERFACE opts-parser.c opts-parser.h)

target_link_libraries(opts-parser INTERFACE OLS::libols)

target_include_directories(opts-parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})