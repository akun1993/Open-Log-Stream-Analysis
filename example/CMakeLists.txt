cmake_minimum_required(VERSION 3.16...3.25)

add_executable(parse_test)


target_sources(parse_test PRIVATE parse_test.cpp)

target_include_directories(parse_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ../libols)

target_link_libraries(parse_test uuid OLS::libols  OLS::scripting dl )

set_target_properties(parse_test PROPERTIES FOLDER example)
