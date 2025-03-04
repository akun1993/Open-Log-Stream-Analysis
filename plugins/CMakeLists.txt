cmake_minimum_required(VERSION 3.16...3.30)

option(ENABLE_PLUGINS "Enable building OLS plugins" ON)

if(NOT ENABLE_PLUGINS)
  set_property(GLOBAL APPEND PROPERTY OLS_FEATURES_DISABLED "Plugin Support")
  return()
endif()

set_property(GLOBAL APPEND PROPERTY OLS_FEATURES_ENABLED "Plugin Support")


# Add plugins in alphabetical order to retain order in IDE projects

add_ols_plugin(file PLATFORMS WINDOWS LINUX MACOS)





