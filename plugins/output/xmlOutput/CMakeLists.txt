project(ols-xml-ouput)

add_library(ols-xml-ouput MODULE)
add_library(ols::xmlOutput ALIAS ols-xml-ouput)

target_link_libraries(ols-xml-ouput PRIVATE OLS::libols OLS::scripting )

set_target_properties(ols-xml-ouput PROPERTIES FOLDER "plugins")

if(OS_WINDOWS)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
        ols-xml-output.rc)
    target_sources(ols-xml-ouput  PRIVATE ols-xml-output.rc)
endif()

target_sources(ols-xml-ouput PRIVATE ols-xml-ouput.cpp)


setup_plugin_target(ols-xml-ouput)
