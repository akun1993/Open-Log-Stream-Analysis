project(ols-data-statistics)

add_library(ols-data-statistics MODULE)
add_library(ols::dataStatistics ALIAS ols-data-statistics)

target_link_libraries(ols-data-statistics PRIVATE OLS::libols )

set_target_properties(ols-data-statistics PROPERTIES FOLDER "plugins")

if(OS_WINDOWS)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
    ols-data-statistics.rc)
    target_sources(ols-data-statistics  PRIVATE ols-data-statistics.rc)
endif()

target_sources(ols-data-statistics PRIVATE ols-data-statistics.cpp)


setup_plugin_target(ols-data-statistics)
