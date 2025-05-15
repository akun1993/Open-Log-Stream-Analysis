add_library(libols-version OBJECT)
add_library(OLS::libols-version ALIAS libols-version)

configure_file(olsversion.c.in olsversion.c @ONLY)

target_sources(libols-version PRIVATE olsversion.c PUBLIC olsversion.h)

target_include_directories(libols-version PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

message("ols-version $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

set_property(TARGET libols-version PROPERTY FOLDER core)

#set_target_properties(libols-version PROPERTIES LINKER_LANGUAGE C)
