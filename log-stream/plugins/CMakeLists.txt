option(ENABLE_PLUGINS "Enable building OLS plugins" ON)
if(NOT ENABLE_PLUGINS)
  ols_status(STATUS "Building with plugins disabled.")
  return()
endif()

add_subdirectory(source)
add_subdirectory(process)
add_subdirectory(output)




