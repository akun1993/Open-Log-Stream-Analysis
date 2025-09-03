cmake_minimum_required(VERSION 3.16...3.21)

option(ENABLE_SCRIPTING_LUA "Enable Lua scripting support" ON)

if(ENABLE_SCRIPTING_LUA)
  add_subdirectory(olslua)
  find_package(Luajit REQUIRED)
else()
  target_disable_feature(ols-scripting "Lua scripting support")
endif()

target_sources(ols-scripting PRIVATE ols-scripting-lua.c ols-scripting-lua.h ols-scripting-lua-source.c)
target_compile_definitions(ols-scripting PUBLIC LUAJIT_FOUND)

add_custom_command(
  OUTPUT swig/swigluarun.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory swig
  COMMAND ${CMAKE_COMMAND} -E env "SWIG_LIB=${SWIG_DIR}" ${SWIG_EXECUTABLE} -lua -external-runtime swig/swigluarun.h
  COMMENT "ols-scripting - generating Luajit SWIG interface headers")

#target_sources(ols-scripting PRIVATE swig/swigluarun.h)
#set_source_files_properties(swig/swigluarun.h PROPERTIES GENERATED ON)
set_source_files_properties(
  ols-scripting-lua.c ols-scripting-lua-source.c
  PROPERTIES COMPILE_OPTIONS "$<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-error=shorten-64-to-32;-Wno-error=shadow>")

target_link_libraries(ols-scripting PRIVATE Luajit::Luajit)
