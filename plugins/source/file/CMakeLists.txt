project(ols-text)

add_library(ols-text MODULE)
add_library(ols::text ALIAS ols-text)

target_link_libraries(ols-text PRIVATE OLS::libols OLS::scripting pcre2-8)

set_target_properties(ols-text PROPERTIES FOLDER "plugins")

if(OS_WINDOWS)
    find_package(7zWindows REQUIRED)
    configure_file(cmake/windows/ols-module.rc.in
                ols-text.rc)
    target_sources(ols-text  PRIVATE ols-text.rc)
endif()

target_sources(ols-text PRIVATE textdocument/ols-text.cpp)


setup_plugin_target(ols-text)
