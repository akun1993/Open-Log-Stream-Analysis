project(ols-xml-output)

add_library(ols-xml-output MODULE)
add_library(ols::xmlOutput ALIAS ols-xml-output)

target_link_libraries(ols-xml-output PRIVATE OLS::libols OLS::scripting tinyxml2)

set_target_properties(ols-xml-output PROPERTIES FOLDER "plugins")

find_program(XSlTPROC REQUIRED)

if(OS_WINDOWS)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
        ols-xml-output.rc)
    target_sources(ols-xml-output  PRIVATE ols-xml-output.rc)
endif()

target_sources(ols-xml-output PRIVATE ols-xml-output.cpp)

setup_plugin_target(ols-xml-output)
