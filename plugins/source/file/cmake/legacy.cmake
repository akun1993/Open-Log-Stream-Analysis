project(ols-text)

add_library(ols-text MODULE)
add_library(OLS::text ALIAS ols-text)

target_link_libraries(ols-text PRIVATE OLS::libols)

set_target_properties(ols-text PROPERTIES FOLDER "plugins")

set(MODULE_DESCRIPTION "OLS GDI+ text module")
configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in ols-text.rc)

target_sources(ols-text PRIVATE gdiplus/ols-text.cpp ols-text.rc)

target_link_libraries(ols-text PRIVATE gdiplus)

target_compile_definitions(ols-text PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)

setup_plugin_target(ols-text)
