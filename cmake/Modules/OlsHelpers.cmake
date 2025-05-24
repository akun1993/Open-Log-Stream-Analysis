# Set OS-specific constants in non-deprecated way
include(GNUInstallDirs)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  include(OlsDefaults_macOS)
  set(OS_MACOS ON)
  set(OS_POSIX ON)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD|OpenBSD")
  include(OlsDefaults_Linux)
  set(OS_POSIX ON)
  string(TOUPPER "${CMAKE_SYSTEM_NAME}" _SYSTEM_NAME_U)
  set(OS_${_SYSTEM_NAME_U} ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  include(OlsDefaults_Windows)
  set(OS_WINDOWS ON)
  set(OS_POSIX OFF)
endif()

# Create global property to hold list of activated modules
set_property(GLOBAL PROPERTY OLS_MODULE_LIST "")

# ######################################################################################################################
# GLOBAL HELPER FUNCTIONS #
# ######################################################################################################################

# Helper function to set up runtime or library targets
function(setup_binary_target target)
  # Set up installation paths for program install
  install(
    TARGETS ${target}
    RUNTIME DESTINATION ${OLS_EXECUTABLE_DESTINATION} COMPONENT ${target}_Runtime
    LIBRARY DESTINATION ${OLS_LIBRARY_DESTINATION}
            COMPONENT ${target}_Runtime
            NAMELINK_COMPONENT ${target}_Development
    ARCHIVE DESTINATION ${OLS_LIBRARY_DESTINATION} COMPONENT ${target}_Development
    PUBLIC_HEADER
      DESTINATION ${OLS_INCLUDE_DESTINATION}
      COMPONENT ${target}_Development
      EXCLUDE_FROM_ALL)

  # Set up installation paths for development rundir
  install(
    TARGETS ${target}
    RUNTIME DESTINATION ${OLS_EXECUTABLE_DESTINATION}
            COMPONENT ols_${target}
            EXCLUDE_FROM_ALL
    LIBRARY DESTINATION ${OLS_LIBRARY_DESTINATION}
            COMPONENT ols_${target}
            EXCLUDE_FROM_ALL
    PUBLIC_HEADER
      DESTINATION ${OLS_INCLUDE_DESTINATION}
      COMPONENT IGNORED
      EXCLUDE_FROM_ALL)

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E env DESTDIR= "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
            ${OLS_OUTPUT_DIR}/$<CONFIG> --component ols_${target} > "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing OLS rundir"
    VERBATIM)

endfunction()

# Helper function to set up OLS plugin targets
function(setup_plugin_target target)
  set_target_properties(${target} PROPERTIES PREFIX "")

  install(
    TARGETS ${target}
    RUNTIME DESTINATION ${OLS_PLUGIN_DESTINATION} COMPONENT ${target}_Runtime
    LIBRARY DESTINATION ${OLS_PLUGIN_DESTINATION}
            COMPONENT ${target}_Runtime
            NAMELINK_COMPONENT ${target}_Development)

  install(
    TARGETS ${target}
    RUNTIME DESTINATION ${OLS_PLUGIN_DESTINATION}
            COMPONENT ols_${target}
            EXCLUDE_FROM_ALL
    LIBRARY DESTINATION ${OLS_PLUGIN_DESTINATION}
            COMPONENT ols_${target}
            EXCLUDE_FROM_ALL)

  setup_target_resources("${target}" "ols-plugins/${target}")
  set_property(GLOBAL APPEND PROPERTY OLS_MODULE_LIST "${target}")
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E env DESTDIR= "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
            ${OLS_OUTPUT_DIR}/$<CONFIG> --component ols_${target} > "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing ${target} to OLS rundir"
    VERBATIM)

  ols_status(ENABLED "${target}")
endfunction()

# Helper function to set up OLS scripting plugin targets
function(setup_script_plugin_target target)
  install(
    TARGETS ${target}
    LIBRARY DESTINATION ${OLS_SCRIPT_PLUGIN_DESTINATION}
            COMPONENT ${target}_Runtime
            NAMELINK_COMPONENT ${target}_Development)

  install(
    TARGETS ${target}
    LIBRARY DESTINATION ${OLS_SCRIPT_PLUGIN_DESTINATION}
            COMPONENT ols_${target}
            EXCLUDE_FROM_ALL)

  if(${target} STREQUAL "olspython")
    install(
      FILES "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.py"
      DESTINATION ${OLS_SCRIPT_PLUGIN_DESTINATION}
      COMPONENT ${target}_Runtime)

    install(
      FILES "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.py"
      DESTINATION ${OLS_SCRIPT_PLUGIN_DESTINATION}
      COMPONENT ols_${target}
      EXCLUDE_FROM_ALL)
  endif()
  set_property(GLOBAL APPEND PROPERTY OLS_SCRIPTING_MODULE_LIST "${target}")
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E env DESTDIR= "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
            ${OLS_OUTPUT_DIR}/$<CONFIG> --component ols_${target} > "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing ${target} to OLS rundir"
    VERBATIM)

  ols_status(ENABLED "${target}")
endfunction()

# Helper function to set up target resources (e.g. L10N files)
function(setup_target_resources target destination)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
      DESTINATION ${OLS_DATA_DESTINATION}/${destination}
      USE_SOURCE_PERMISSIONS
      COMPONENT ${target}_Runtime)

    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
      DESTINATION ${OLS_DATA_DESTINATION}/${destination}
      USE_SOURCE_PERMISSIONS
      COMPONENT ols_${target}
      EXCLUDE_FROM_ALL)
  endif()
endfunction()

# Helper function to set up specific resource files for targets
function(add_target_resource target resource destination)
  install(
    FILES ${resource}
    DESTINATION ${OLS_DATA_DESTINATION}/${destination}
    COMPONENT ${target}_Runtime)

  install(
    FILES ${resource}
    DESTINATION ${OLS_DATA_DESTINATION}/${destination}
    COMPONENT ols_${target}
    EXCLUDE_FROM_ALL)
endfunction()

# Helper function to set up OLS app target
function(setup_ols_app target)
  setup_binary_target(${target})

  get_property(OLS_MODULE_LIST GLOBAL PROPERTY OLS_MODULE_LIST)
  list(LENGTH OLS_MODULE_LIST _LEN)
  if(_LEN GREATER 0)
    add_dependencies(${target} ${OLS_MODULE_LIST})
  endif()

  get_property(OLS_SCRIPTING_MODULE_LIST GLOBAL PROPERTY OLS_SCRIPTING_MODULE_LIST)
  list(LENGTH OLS_SCRIPTING_MODULE_LIST _LEN)
  if(_LEN GREATER 0)
    add_dependencies(${target} ${OLS_SCRIPTING_MODULE_LIST})
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E env DESTDIR= "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
            ${OLS_OUTPUT_DIR}/$<CONFIG> --component ols_rundir > "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing OLS rundir"
    VERBATIM)
endfunction()



# Helper function to export target to build and install tree. Allows usage of `find_package(libols)` by other build
# trees
function(export_target target)
  set(CMAKE_EXPORT_PACKAGE_REGISTRY OFF)

  if(OS_LINUX OR OS_FREEBSD)
    set(_EXCLUDE "")
  else()
    set(_EXCLUDE "EXCLUDE_FROM_ALL")
  endif()
  install(
    TARGETS ${target}
    EXPORT ${target}Targets
    RUNTIME DESTINATION ${OLS_EXECUTABLE_DESTINATION}
            COMPONENT ols_libraries
            ${_EXCLUDE}
    LIBRARY DESTINATION ${OLS_LIBRARY_DESTINATION}
            COMPONENT ols_libraries
            ${_EXCLUDE}
    ARCHIVE DESTINATION ${OLS_LIBRARY_DESTINATION}
            COMPONENT ols_libraries
            ${_EXCLUDE}
    INCLUDES
    DESTINATION ${OLS_INCLUDE_DESTINATION}
    PUBLIC_HEADER
      DESTINATION ${OLS_INCLUDE_DESTINATION}
      COMPONENT ols_libraries
      ${_EXCLUDE})

  get_target_property(target_type ${target} TYPE)
  if(NOT target_type STREQUAL INTERFACE_LIBRARY)
    include(GenerateExportHeader)
    generate_export_header(${target} EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h)

    target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h)
  endif()

  set(TARGETS_EXPORT_NAME "${target}Targets")
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${target}Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake
    INSTALL_DESTINATION ${OLS_CMAKE_DESTINATION}/${target}
    PATH_VARS OLS_PLUGIN_DESTINATION OLS_DATA_DESTINATION)

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    VERSION ${OLS_VERSION_CANONICAL}
    COMPATIBILITY SameMajorVersion)

  export(
    EXPORT ${target}Targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE OLS::)

  export(PACKAGE "${target}")

  install(
    EXPORT ${TARGETS_EXPORT_NAME}
    FILE ${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE OLS::
    DESTINATION ${OLS_CMAKE_DESTINATION}/${target}
    COMPONENT ols_libraries
    ${_EXCLUDE})

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    DESTINATION ${OLS_CMAKE_DESTINATION}/${target}
    COMPONENT ols_libraries
    ${_EXCLUDE})
endfunction()


if(NOT QT_VERSION)
  set(QT_VERSION
      AUTO
      CACHE STRING "OLS Qt version [AUTO, 5, 6]" FORCE)
  set_property(CACHE QT_VERSION PROPERTY STRINGS AUTO 5 6)
endif()

macro(find_qt)
  set(multiValueArgs COMPONENTS COMPONENTS_WIN COMPONENTS_MAC COMPONENTS_LINUX)
  cmake_parse_arguments(FIND_QT "" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})
  set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)
  find_package(
    Qt5
    COMPONENTS Core
    QUIET)
  find_package(
    Qt6
    COMPONENTS Core
    QUIET)

  if(NOT _QT_VERSION AND QT_VERSION STREQUAL AUTO)
    if(TARGET Qt6::Core)
      set(_QT_VERSION
          6
          CACHE INTERNAL "")
    elseif(TARGET Qt5::Core)
      set(_QT_VERSION
          5
          CACHE INTERNAL "")
    endif()
    ols_status(STATUS "Qt version: ${_QT_VERSION}")
  elseif(NOT _QT_VERSION)
    if(TARGET Qt${QT_VERSION}::Core)
      set(_QT_VERSION
          ${QT_VERSION}
          CACHE INTERNAL "")
    else()
      if(QT_VERSION EQUAL 6)
        set(FALLBACK_QT_VERSION 5)
      else()
        set(FALLBACK_QT_VERSION 6)
      endif()
      message(
        WARNING
          "Qt${QT_VERSION} was not found, falling back to Qt${FALLBACK_QT_VERSION}"
      )

      if(TARGET Qt${FALLBACK_QT_VERSION}::Core)
        set(_QT_VERSION
            ${FALLBACK_QT_VERSION}
            CACHE INTERNAL "")
      endif()
    endif()
    ols_status(STATUS "Qt version: ${_QT_VERSION}")
  endif()

  set(QT_NO_CREATE_VERSIONLESS_TARGETS OFF)

  if(NOT _QT_VERSION)
    message(FATAL_ERROR "Neither Qt5 or Qt6 were found")
  endif()

  if(OS_WINDOWS)
    find_package(
      Qt${_QT_VERSION}
      COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_WIN}
      REQUIRED)
  elseif(OS_MACOS)
    find_package(
      Qt${_QT_VERSION}
      COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_MAC}
      REQUIRED)
  else()
    find_package(
      Qt${_QT_VERSION}
      COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_LINUX}
      REQUIRED)
  endif()

  list(APPEND FIND_QT_COMPONENTS "Core")

  if("Gui" IN_LIST FIND_QT_COMPONENTS_LINUX)
    list(APPEND FIND_QT_COMPONENTS_LINUX "GuiPrivate")
  endif()

  foreach(_COMPONENT IN LISTS FIND_QT_COMPONENTS FIND_QT_COMPONENTS_WIN
                              FIND_QT_COMPONENTS_MAC FIND_QT_COMPONENTS_LINUX)
    if(NOT TARGET Qt::${_COMPONENT} AND TARGET Qt${_QT_VERSION}::${_COMPONENT})

      add_library(Qt::${_COMPONENT} INTERFACE IMPORTED)
      set_target_properties(
        Qt::${_COMPONENT} PROPERTIES INTERFACE_LINK_LIBRARIES
                                     "Qt${_QT_VERSION}::${_COMPONENT}")
    endif()
  endforeach()
endmacro()

# Idea adapted from: https://github.com/edsiper/cmake-options
macro(set_option option value)
  set(${option}
      ${value}
      CACHE INTERNAL "")
endmacro()

function(ols_status status text)
  set(_OLS_STATUS_DISABLED "OLS:  DISABLED   ")
  set(_OLS_STATUS_ENABLED "OLS:  ENABLED    ")
  set(_OLS_STATUS "OLS:  ")
  if(status STREQUAL "DISABLED")
    message(STATUS "${_OLS_STATUS_DISABLED}${text}")
  elseif(status STREQUAL "ENABLED")
    message(STATUS "${_OLS_STATUS_ENABLED}${text}")
  else()
    message(${status} "${_OLS_STATUS}${text}")
  endif()
endfunction()

if(OS_WINDOWS)
  include(OlsHelpers_Windows)
elseif(OS_MACOS)
  include(OlsHelpers_macOS)
elseif(OS_POSIX)
  include(OlsHelpers_Linux)
endif()

# ######################################################################################################################
# LEGACY FALLBACKS     #
# ######################################################################################################################

# Helper function to install OLS plugin with associated resource directory
function(_install_ols_plugin_with_data target source)
  setup_plugin_target(${target})

  if(NOT ${source} STREQUAL "data"
     AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
     AND NOT OS_MACOS)
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${source}/
      DESTINATION ${OLS_DATA_DESTINATION}/ols-plugins/${target}
      USE_SOURCE_PERMISSIONS
      COMPONENT ${target}_Runtime)

    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${source}/
      DESTINATION ${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_DATA_DESTINATION}/ols-plugins/${target}
      COMPONENT ols_${target}
      EXCLUDE_FROM_ALL)

    if(OS_WINDOWS AND DEFINED ENV{olsInstallerTempDir})
      install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${source}/
        DESTINATION $ENV{olsInstallerTempDir}/${OLS_DATA_DESTINATION}/ols-plugins/${target}
        COMPONENT ols_${target}
        EXCLUDE_FROM_ALL)
    endif()
  endif()
endfunction()

# Helper function to install OLS plugin
function(_install_ols_plugin target)
  setup_plugin_target(${target})
endfunction()

# Helper function to install data for a target only
function(_install_ols_datatarget target destination)
  install(
    TARGETS ${target}
    LIBRARY DESTINATION ${OLS_DATA_DESTINATION}/${destination}
            COMPONENT ${target}_Runtime
            NAMELINK_COMPONENT ${target}_Development
    RUNTIME DESTINATION ${OLS_DATA_DESTINATION}/${destination} COMPONENT ${target}_Runtime)

  install(
    TARGETS ${target}
    LIBRARY DESTINATION ${OLS_DATA_DESTINATION}/${destination}
            COMPONENT ols_${target}
            EXCLUDE_FROM_ALL
    RUNTIME DESTINATION ${OLS_DATA_DESTINATION}/${destination}
            COMPONENT ols_${target}
            EXCLUDE_FROM_ALL)

  if(OS_WINDOWS)
    if(MSVC)
      add_target_resource(${target} "$<TARGET_PDB_FILE:${target}>" "${destination}" OPTIONAL)
    endif()

    if(DEFINED ENV{olsInstallerTempDir})
      install(
        TARGETS ${target}
        RUNTIME
          DESTINATION $ENV{olsInstallerTempDir}/${OLS_DATA_DESTINATION}/${destination}/$<TARGET_FILE_NAME:${target}>
          COMPONENT ols_${target}
          EXCLUDE_FROM_ALL
        LIBRARY
          DESTINATION $ENV{olsInstallerTempDir}/${OLS_DATA_DESTINATION}/${destination}/$<TARGET_FILE_NAME:${target}>
          COMPONENT ols_${target}
          EXCLUDE_FROM_ALL)
    endif()
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E env DESTDIR= "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix
            ${OLS_OUTPUT_DIR}/$<CONFIG> --component ols_${target} > "$<IF:$<PLATFORM_ID:Windows>,nul,/dev/null>"
    COMMENT "Installing ${target} to OLS rundir"
    VERBATIM)
endfunction()

# legacy_check: Macro to check for CMake framework version and include legacy list file
macro(legacy_check)
  if(OLS_CMAKE_VERSION VERSION_LESS 3.0.0)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/legacy.cmake)
      include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/legacy.cmake)
    endif()
    return()
  endif()
endmacro()
