project(ols-xml-output)

add_library(ols-xml-output MODULE)
add_library(ols::xmlOutput ALIAS ols-xml-output)

target_link_libraries(ols-xml-output PRIVATE OLS::libols OLS::scripting tinyxml2)

set_target_properties(ols-xml-output PROPERTIES
    FOLDER "plugins"
    INSTALL_RPATH "$ORIGIN/.."
    BUILD_RPATH "$ORIGIN/.."
)

if(OS_WINDOWS)
    find_package(XSlTPROCWindows REQUIRED)
    configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
        ols-xml-output.rc)
    target_sources(ols-xml-output  PRIVATE ols-xml-output.rc)
elseif(OS_POSIX)
    find_program(XSLTPROC NAMES xsltproc)
endif()

target_sources(ols-xml-output PRIVATE ols-xml-output.cpp)

setup_plugin_target(ols-xml-output)

# Install template.xslt as plugin data resource so ols_find_module_file can locate it
add_target_resource(ols-xml-output ${CMAKE_CURRENT_SOURCE_DIR}/template.xslt "ols-plugins/ols-xml-output")
