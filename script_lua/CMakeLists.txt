cmake_minimum_required(VERSION 3.16...3.25)

# Install Lua script resources into the OLS executable rundir.
# They will be available under bin/<arch>/script_lua.
setup_script_resources(${CMAKE_CURRENT_SOURCE_DIR} script_lua "*.lua")
