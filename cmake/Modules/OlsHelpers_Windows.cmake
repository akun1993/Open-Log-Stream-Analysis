# Helper function to set up runtime or library targets
function(setup_binary_target target)
  _setup_binary_target(${ARGV})

  if(DEFINED ENV{OLS_InstallerTempDir})
    install(
      TARGETS ${target}
      RUNTIME DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_EXECUTABLE_DESTINATION}
              COMPONENT ols_${target}
              EXCLUDE_FROM_ALL
      LIBRARY DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_LIBRARY_DESTINATION}
              COMPONENT ols_${target}
              EXCLUDE_FROM_ALL
      PUBLIC_HEADER
        DESTINATION ${OLS_INCLUDE_DESTINATION}
        COMPONENT ols_${target}
        EXCLUDE_FROM_ALL)

    if(MSVC)
      install(
        FILES $<TARGET_PDB_FILE:${target}>
        CONFIGURATIONS "RelWithDebInfo" "Debug"
        DESTINATION
          $ENV{OLS_InstallerTempDir}/$<IF:$<STREQUAL:$<TARGET_PROPERTY:${target},TYPE>,EXECUTABLE>,${OLS_EXECUTABLE_DESTINATION},${OLS_LIBRARY_DESTINATION}>
        COMPONENT ols_${target}
        OPTIONAL EXCLUDE_FROM_ALL)
    endif()
  endif()

  if(MSVC)
    target_link_options(${target} PRIVATE /PDBALTPATH:$<TARGET_PDB_FILE_NAME:${target}>)

    install(
      FILES $<TARGET_PDB_FILE:${target}>
      CONFIGURATIONS "RelWithDebInfo" "Debug"
      DESTINATION
        $<IF:$<STREQUAL:$<TARGET_PROPERTY:${target},TYPE>,EXECUTABLE>,${OLS_EXECUTABLE_DESTINATION},${OLS_LIBRARY_DESTINATION}>
      COMPONENT ${target}_Runtime
      OPTIONAL)

    install(
      FILES $<TARGET_PDB_FILE:${target}>
      CONFIGURATIONS "RelWithDebInfo" "Debug"
      DESTINATION
        $<IF:$<STREQUAL:$<TARGET_PROPERTY:${target},TYPE>,EXECUTABLE>,${OLS_EXECUTABLE_DESTINATION},${OLS_LIBRARY_DESTINATION}>
      COMPONENT ols_${target}
      OPTIONAL EXCLUDE_FROM_ALL)
  endif()

  if(${target} STREQUAL "libols")
    setup_libols_target(${target})
  endif()
endfunction()

# Helper function to set up OLS plugin targets
function(setup_plugin_target target)
  _setup_plugin_target(${ARGV})

  if(MSVC)
    target_link_options(${target} PRIVATE /PDBALTPATH:$<TARGET_PDB_FILE_NAME:${target}>)

    install(
      FILES $<TARGET_PDB_FILE:${target}>
      CONFIGURATIONS "RelWithDebInfo" "Debug"
      DESTINATION ${OLS_PLUGIN_DESTINATION}
      COMPONENT ${target}_Runtime
      OPTIONAL)

    install(
      FILES $<TARGET_PDB_FILE:${target}>
      CONFIGURATIONS "RelWithDebInfo" "Debug"
      DESTINATION ${OLS_PLUGIN_DESTINATION}
      COMPONENT ols_${target}
      OPTIONAL EXCLUDE_FROM_ALL)
  endif()

  if(DEFINED ENV{OLS_InstallerTempDir})
    install(
      TARGETS ${target}
      RUNTIME DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_PLUGIN_DESTINATION}
              COMPONENT ols_${target}
              EXCLUDE_FROM_ALL
      LIBRARY DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_PLUGIN_DESTINATION}
              COMPONENT ols_${target}
              EXCLUDE_FROM_ALL)

    if(MSVC)
      install(
        FILES $<TARGET_PDB_FILE:${target}>
        CONFIGURATIONS "RelWithDebInfo" "Debug"
        DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_PLUGIN_DESTINATION}
        COMPONENT ols_${target}
        OPTIONAL EXCLUDE_FROM_ALL)
    endif()
  endif()
endfunction()

# Helper function to set up OLS scripting plugin targets
function(setup_script_plugin_target target)
  _setup_script_plugin_target(${ARGV})

  if(MSVC)
    target_link_options(${target} PRIVATE /PDBALTPATH:$<TARGET_PDB_FILE_NAME:${target}>)

    install(
      FILES $<TARGET_PDB_FILE:${target}>
      CONFIGURATIONS "RelWithDebInfo" "Debug"
      DESTINATION ${OLS_SCRIPT_PLUGIN_DESTINATION}
      COMPONENT ols_${target}
      OPTIONAL EXCLUDE_FROM_ALL)
  endif()

  if(DEFINED ENV{OLS_InstallerTempDir})
    install(
      TARGETS ${target}
      RUNTIME DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_SCRIPT_PLUGIN_DESTINATION}
              COMPONENT ols_${target}
              EXCLUDE_FROM_ALL
      LIBRARY DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_SCRIPT_PLUGIN_DESTINATION}
              COMPONENT ols_${target}
              EXCLUDE_FROM_ALL)

    if(MSVC)
      install(
        FILES $<TARGET_PDB_FILE:${target}>
        CONFIGURATIONS "RelWithDebInfo" "Debug"
        DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_SCRIPT_PLUGIN_DESTINATION}
        COMPONENT ols_${target}
        OPTIONAL EXCLUDE_FROM_ALL)
    endif()

    if(${target} STREQUAL "olspython" AND ${_ARCH_SUFFIX} EQUAL 64)
      install(
        FILES "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.py"
        DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_SCRIPT_PLUGIN_DESTINATION}
        COMPONENT ols_${target}
        EXCLUDE_FROM_ALL)
    endif()
  endif()
endfunction()

# Helper function to set up target resources (e.g. L10N files)
function(setup_target_resources target destination)
  _setup_target_resources(${ARGV})

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    if(${_ARCH_SUFFIX} EQUAL 64 AND DEFINED ENV{OLS_InstallerTempDir})

      install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
        DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_DATA_DESTINATION}/${destination}
        USE_SOURCE_PERMISSIONS
        COMPONENT ols_${target}
        EXCLUDE_FROM_ALL)
    endif()
  endif()
endfunction()

# Helper function to set up specific resource files for targets
function(add_target_resource)
  set(target ${ARGV0})
  set(resource ${ARGV1})
  set(destination ${ARGV2})
  if(${ARGC} EQUAL 4)
    set(optional ${ARGV3})
  else()
    set(optional "")
  endif()

  install(
    FILES ${resource}
    DESTINATION ${OLS_DATA_DESTINATION}/${destination}
    COMPONENT ${target}_Runtime
    ${optional})

  install(
    FILES ${resource}
    DESTINATION ${OLS_DATA_DESTINATION}/${destination}
    COMPONENT ols_${target}
    ${optional} EXCLUDE_FROM_ALL)

  if(DEFINED ENV{OLS_InstallerTempDir})
    install(
      FILES ${resource}
      DESTINATION $ENV{OLS_InstallerTempDir}/${OLS_DATA_DESTINATION}/${destination}
      COMPONENT ols_${target}
      ${optional} EXCLUDE_FROM_ALL)
  endif()
endfunction()

# Helper function to set up OLS app target
function(setup_ols_app target)
  # detect outdated ols-browser submodule


  _setup_ols_app(${ARGV})

  if(MSVC)
    include(CopyMSVCBins)
  endif()
endfunction()

# Helper function to export target to build and install tree. Allows usage of `find_package(libols)` by other build
# trees
function(export_target target)
  set(CMAKE_EXPORT_PACKAGE_REGISTRY OFF)

  install(
    TARGETS ${target}
    EXPORT ${target}Targets
    RUNTIME DESTINATION "${OLS_EXECUTABLE_EXPORT_DESTINATION}"
            COMPONENT ols_libraries
            EXCLUDE_FROM_ALL
    LIBRARY DESTINATION "${OLS_LIBRARY_EXPORT_DESTINATION}"
            COMPONENT ols_libraries
            EXCLUDE_FROM_ALL
    ARCHIVE DESTINATION "${OLS_LIBRARY_EXPORT_DESTINATION}"
            COMPONENT ols_libraries
            EXCLUDE_FROM_ALL
    INCLUDES
    DESTINATION "${OLS_INCLUDE_DESTINATION}"
    PUBLIC_HEADER
      DESTINATION "${OLS_INCLUDE_DESTINATION}"
      COMPONENT ols_libraries
      EXCLUDE_FROM_ALL)

  get_target_property(target_type ${target} TYPE)
  if(MSVC AND NOT target_type STREQUAL INTERFACE_LIBRARY)
    install(
      FILES $<TARGET_PDB_FILE:${target}>
      CONFIGURATIONS "RelWithDebInfo" "Debug"
      DESTINATION "${OLS_EXECUTABLE_EXPORT_DESTINATION}"
      COMPONENT ols_libraries
      OPTIONAL EXCLUDE_FROM_ALL)
  endif()

  include(GenerateExportHeader)
  generate_export_header(${target} EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h")

  target_sources(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h")

  set(TARGETS_EXPORT_NAME "${target}Targets")
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${target}Config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake"
    INSTALL_DESTINATION ${OLS_CMAKE_DESTINATION}
    PATH_VARS OLS_PLUGIN_DESTINATION OLS_DATA_DESTINATION)

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    VERSION ${OLS_VERSION_CANONICAL}
    COMPATIBILITY SameMajorVersion)

  export(
    EXPORT ${target}Targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGETS_EXPORT_NAME}.cmake"
    NAMESPACE OLS::)

  export(PACKAGE "${target}")

  install(
    EXPORT ${TARGETS_EXPORT_NAME}
    FILE ${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE OLS::
    DESTINATION ${OLS_CMAKE_DESTINATION}
    COMPONENT ols_libraries
    EXCLUDE_FROM_ALL)

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    DESTINATION ${OLS_CMAKE_DESTINATION}
    COMPONENT ols_libraries
    EXCLUDE_FROM_ALL)
endfunction()


# Helper function to gather external libraries depended-on by libols
function(setup_libols_target target)
  set(_ADDITIONAL_FILES "${CMAKE_SOURCE_DIR}/additional_install_files")

  if(DEFINED ENV{OLS_AdditionalInstallFiles})
    set(_ADDITIONAL_FILES "$ENV{OLS_AdditionalInstallFiles}")
  endif()

  if(NOT INSTALLER_RUN)
    list(APPEND _LIBOLS_FIXUPS "misc:." "data:${OLS_DATA_DESTINATION}" "libs${_ARCH_SUFFIX}:${OLS_LIBRARY_DESTINATION}"
         "exec${_ARCH_SUFFIX}:${OLS_EXECUTABLE_DESTINATION}")
  else()
    list(
      APPEND
      _LIBOLS_FIXUPS
      "misc:."
      "data:${OLS_DATA_DESTINATION}"
      "libs32:${OLS_LIBRARY32_DESTINATION}"
      "libs64:${OLS_LIBRARY64_DESTINATION}"
      "exec32:${OLS_EXECUTABLE32_DESTINATION}"
      "exec64:${OLS_EXECUTABLE64_DESTINATION}")
  endif()

  foreach(_FIXUP IN LISTS _LIBOLS_FIXUPS)
    string(REPLACE ":" ";" _FIXUP ${_FIXUP})
    list(GET _FIXUP 0 _SOURCE)
    list(GET _FIXUP 1 _DESTINATION)

    install(
      DIRECTORY ${_ADDITIONAL_FILES}/${_SOURCE}/
      DESTINATION ${_DESTINATION}
      USE_SOURCE_PERMISSIONS
      COMPONENT ${target}_Runtime
      PATTERN ".gitignore" EXCLUDE)

    install(
      DIRECTORY ${_ADDITIONAL_FILES}/${_SOURCE}/
      DESTINATION ${_DESTINATION}
      USE_SOURCE_PERMISSIONS
      COMPONENT ols_rundir
      EXCLUDE_FROM_ALL
      PATTERN ".gitignore" EXCLUDE)

    if(_SOURCE MATCHES "(libs|exec)(32|64)?")
      install(
        DIRECTORY ${_ADDITIONAL_FILES}/${_SOURCE}$<IF:$<CONFIG:Debug>,d,r>/
        DESTINATION ${_DESTINATION}
        USE_SOURCE_PERMISSIONS
        COMPONENT ${target}_Runtime
        PATTERN ".gitignore" EXCLUDE)

      install(
        DIRECTORY ${_ADDITIONAL_FILES}/${_SOURCE}$<IF:$<CONFIG:Debug>,d,r>/
        DESTINATION ${_DESTINATION}
        USE_SOURCE_PERMISSIONS
        COMPONENT ols_rundir
        EXCLUDE_FROM_ALL
        PATTERN ".gitignore" EXCLUDE)
    endif()
  endforeach()
endfunction()

# Helper function to compile artifacts for multi-architecture installer
function(generate_multiarch_installer)
  if(NOT DEFINED ENV{OLS_InstallerTempDir} AND NOT DEFINED ENV{olsInstallerTempDir})
    ols_status(FATAL_ERROR
               "Function generate_multiarch_installer requires environment variable 'OLS_InstallerTempDir' to be set")
  endif()

  add_custom_target(installer_files ALL)

  setup_libols_target(installer_files)

  install(
    DIRECTORY "$ENV{OLS_InstallerTempDir}/"
    DESTINATION "."
    USE_SOURCE_PERMISSIONS)
endfunction()

# Helper function to install header files
function(install_headers target)
  install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
    DESTINATION ${OLS_INCLUDE_DESTINATION}
    COMPONENT ols_libraries
    EXCLUDE_FROM_ALL FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "ols-hevc.h" EXCLUDE
    PATTERN "ols-nix-*.h" EXCLUDE
    PATTERN "*-posix.h" EXCLUDE
    PATTERN "util/apple" EXCLUDE
    PATTERN "cmake" EXCLUDE
    PATTERN "pkgconfig" EXCLUDE
    PATTERN "data" EXCLUDE)

  if(NOT EXISTS "${OLS_INCLUDE_DESTINATION}/olsconfig.h")
    install(
      FILES "${CMAKE_BINARY_DIR}/config/olsconfig.h"
      DESTINATION "${OLS_INCLUDE_DESTINATION}"
      COMPONENT ols_libraries
      EXCLUDE_FROM_ALL)
  endif()
endfunction()
