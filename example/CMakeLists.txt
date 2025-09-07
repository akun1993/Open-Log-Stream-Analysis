cmake_minimum_required(VERSION 3.16...3.25)

add_executable(parse_test)

target_sources(parse_test PRIVATE parse_test.cpp)

target_include_directories(parse_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ../libols)


if(OS_WINDOWS)
    target_link_libraries(parse_test uuid OLS::libols OLS::scripting)
else()
    target_link_libraries(parse_test uuid OLS::libols OLS::scripting dl)
endif()

set_target_properties(parse_test PROPERTIES FOLDER "example")

#setup_binary_target(parse_test)
setup_ols_app(parse_test)