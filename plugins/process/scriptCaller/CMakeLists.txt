project(ols-script-caller)

add_library(ols-script-caller MODULE)
add_library(ols::scriptCaller ALIAS ols-script-caller)

target_link_libraries(ols-script-caller PRIVATE OLS::libols OLS::scripting  OLS::uthash)

set_target_properties(ols-script-caller PROPERTIES FOLDER "plugins")

if(OS_WINDOWS)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
        ols-script-caller.rc)
    target_sources(ols-script-caller  PRIVATE ols-script-caller.rc)
endif()

target_sources(ols-script-caller PRIVATE ols-script-caller)


setup_plugin_target(ols-script-caller)
