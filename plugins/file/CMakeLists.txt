cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

add_library(ols-text MODULE)
add_library(ols::text ALIAS ols-text)

configure_file(cmake/windows/osl-module.rc.in ols-text.rc)

target_sources(ols-text PRIVATE gdiplus/ols-text.cpp ols-text.rc)

target_link_libraries(ols-text PRIVATE OLS::libols gdiplus)

# cmake-format: off
set_target_properties_obs(ols-text PROPERTIES FOLDER plugins PREFIX "")
# cmake-format: on
