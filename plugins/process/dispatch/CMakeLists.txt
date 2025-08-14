project(ols-data-dispatch)

add_library(ols-data-dispatch MODULE)
add_library(ols::dataDispatch ALIAS ols-data-dispatch)

target_link_libraries(ols-data-dispatch PRIVATE OLS::libols )

set_target_properties(ols-data-dispatch PROPERTIES FOLDER "plugins")

if(OS_WINDOWS)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
    ols-data-dispatch.rc)
    target_sources(ols-data-dispatch  PRIVATE ols-data-dispatch.rc)
endif()

target_sources(ols-data-dispatch PRIVATE ols-data-dispatch.cpp)


setup_plugin_target(ols-data-dispatch)
