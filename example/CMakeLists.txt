cmake_minimum_required(VERSION 3.16...3.25)

add_executable(parse_test)

target_sources(parse_test PRIVATE parse_test.cpp)

# 使用现代 CMake 方式链接库，OLS::archive 已经包含了头文件目录
target_link_libraries(parse_test PRIVATE
    OLS::libols
    OLS::scripting
    OLS::archive
    Jansson::Jansson
    pcre2-8
)

# 平台特定依赖
if(OS_WINDOWS)
    target_link_libraries(parse_test PRIVATE uuid)
else()
    target_link_libraries(parse_test PRIVATE dl)
endif()

# MSVC 编译选项
if(MSVC)
    target_compile_options(parse_test PRIVATE /utf-8)
endif()

set_target_properties(parse_test PROPERTIES FOLDER "example")

setup_ols_app(parse_test)
