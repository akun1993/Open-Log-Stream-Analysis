cmake_minimum_required(VERSION 3.20)

# Enable modern cmake policies
if(POLICY CMP0009)
  cmake_policy(SET CMP0009 NEW)
endif()

if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      "${CMAKE_BINARY_DIR}/rundir"
      CACHE STRING "Directory to install OLS after building" FORCE)
endif()

# Enable building Windows modules with file descriptors
# https://github.com/olsproject/ols-studio/commit/51be039cf82fc347587d16b48f74e65e86bee301
set(MODULE_DESCRIPTION "OLS Studio")

macro(setup_ols_project)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_ARCH_SUFFIX 64)
  else()
    set(_ARCH_SUFFIX 32)
  endif()

  set(OLS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/rundir")

  set(OLS_EXECUTABLE_DESTINATION "bin/${_ARCH_SUFFIX}bit")
  set(OLS_EXECUTABLE32_DESTINATION "bin/32bit")
  set(OLS_EXECUTABLE64_DESTINATION "bin/64bit")
  set(OLS_LIBRARY_DESTINATION "bin/${_ARCH_SUFFIX}bit")
  set(OLS_LIBRARY32_DESTINATION "bin/32bit")
  set(OLS_LIBRARY64_DESTINATION "bin/64bit")

  set(OLS_EXECUTABLE_EXPORT_DESTINATION "bin/${_ARCH_SUFFIX}bit")
  set(OLS_LIBRARY_EXPORT_DESTINATION "bin/${_ARCH_SUFFIX}bit")

  set(OLS_PLUGIN_DESTINATION "ols-plugins/${_ARCH_SUFFIX}bit")
  set(OLS_PLUGIN_PATH "../../${OLS_PLUGIN_DESTINATION}")
  set(OLS_PLUGIN32_DESTINATION "ols-plugins/32bit")
  set(OLS_PLUGIN64_DESTINATION "ols-plugins/64bit")

  set(OLS_INCLUDE_DESTINATION "include")
  set(OLS_CMAKE_DESTINATION "cmake")
  set(OLS_DATA_DESTINATION "data")
  set(OLS_DATA_PATH "../../${OLS_DATA_DESTINATION}")
  set(OLS_INSTALL_PREFIX "")

  set(OLS_SCRIPT_PLUGIN_DESTINATION "${OLS_DATA_DESTINATION}/ols-scripting/${_ARCH_SUFFIX}bit")
  set(OLS_SCRIPT_PLUGIN_PATH "../../${OLS_SCRIPT_PLUGIN_DESTINATION}")

  string(REPLACE "-" ";" UI_VERSION_SPLIT ${OLS_VERSION})
  list(GET UI_VERSION_SPLIT 0 UI_VERSION)
  string(REPLACE "." ";" UI_VERSION_SEMANTIC ${UI_VERSION})
  list(GET UI_VERSION_SEMANTIC 0 UI_VERSION_MAJOR)
  list(GET UI_VERSION_SEMANTIC 1 UI_VERSION_MINOR)
  list(GET UI_VERSION_SEMANTIC 2 UI_VERSION_PATCH)

  if(INSTALLER_RUN
     AND NOT DEFINED ENV{OLS_InstallerTempDir}
     AND NOT DEFINED ENV{olsInstallerTempDir})
    message(FATAL_ERROR "Environment variable olsInstallerTempDir is needed for multiarch installer generation")
  endif()

  if(DEFINED ENV{OLS_DepsPath${_ARCH_SUFFIX}})
    set(DepsPath${_ARCH_SUFFIX} "$ENV{OLS_DepsPath${_ARCH_SUFFIX}}")
  elseif(DEFINED ENV{OLS_DepsPath})
    set(DepsPath "$ENV{DepsPath}")
  elseif(DEFINED ENV{DepsPath${_ARCH_SUFFIX}})
    set(DepsPath${_ARCH_SUFFIX} "$ENV{DepsPath${_ARCH_SUFFIX}}")
  elseif(DEFINED ENV{DepsPath})
    set(DepsPath "$ENV{DepsPath}")
  endif()

  if(DEFINED ENV{OLS_QTDIR${_ARCH_SUFFIX}})
    set(QTDIR${_ARCH_SUFFIX} "$ENV{OLS_QTDIR${_ARCH_SUFFIX}}")
  elseif(DEFINED ENV{OLS_QTDIR})
    set(QTDIR "$ENV{OLS_QTDIR}")
  elseif(DEFINED ENV{QTDIR${_ARCH_SUFFIX}})
    set(QTDIR${_ARCH_SUFFIX} "$ENV{QTDIR${_ARCH_SUFFIX}}")
  elseif(DEFINED ENV{QTDIR})
    set(QTDIR "$ENV{QTDIR}")
  endif()

  if(DEFINED DepsPath${_ARCH_SUFFIX})
    list(APPEND CMAKE_PREFIX_PATH "${DepsPath${_ARCH_SUFFIX}}" "${DepsPath${_ARCH_SUFFIX}}/bin")
  elseif(DEFINED DepsPath)
    list(APPEND CMAKE_PREFIX_PATH "${DepsPath}" "${DepsPath}/bin")
  elseif(NOT DEFINED CMAKE_PREFIX_PATH)
    message(
      WARNING "No CMAKE_PREFIX_PATH set: OLS requires pre-built dependencies for building on Windows."
              "Please download the appropriate ols-deps package for your architecture and set CMAKE_PREFIX_PATH "
              "to the base directory and 'bin' directory inside it:\n"
              "CMAKE_PREFIX_PATH=\"<PATH_TO_OLS_DEPS>\"\n"
              "Download pre-built OLS dependencies at https://github.com/olsproject/ols-deps/releases\n")
  endif()

  if(DEFINED QTDIR${_ARCH_SUFFIX})
    list(APPEND CMAKE_PREFIX_PATH "${QTDIR${_ARCH_SUFFIX}}")
  elseif(DEFINED QTDIR)
    list(APPEND CMAKE_PREFIX_PATH "${QTDIR}")
  endif()

  if(DEFINED ENV{VLCPath})
    set(VLCPath "$ENV{VLCPath}")
  elseif(DEFINED ENV{OLS_VLCPath})
    set(VLCPath "$ENV{OLS_VLCPath}")
  endif()

  if(DEFINED VLCPath)
    set(VLC_PATH "${VLCPath}")
  endif()

  if(DEFINED ENV{CEF_ROOT_DIR})
    set(CEF_ROOT_DIR "$ENV{CEF_ROOT_DIR}")
  elseif(DEFINED ENV{OLS_CEF_ROOT_DIR})
    set(CEF_ROOT_DIR "$ENV{OLS_CEF_ROOT_DIR}")
  endif()

  if(DEFINED ENV{OLS_InstallerTempDir})
    file(TO_CMAKE_PATH "$ENV{OLS_InstallerTempDir}" _INSTALLER_TEMP_DIR)
  elseif(DEFINED ENV{olsInstallerTempDir})
    file(TO_CMAKE_PATH "$ENV{olsInstallerTempDir}" _INSTALLER_TEMP_DIR)
  endif()

  set(ENV{OLS_InstallerTempDir} "${_INSTALLER_TEMP_DIR}")
  unset(_INSTALLER_TEMP_DIR)

  if(DEFINED ENV{OLS_AdditionalInstallFiles})
    file(TO_CMAKE_PATH "$ENV{OLS_AdditionalInstallFiles}" _ADDITIONAL_FILES)
  elseif(DEFINED ENV{olsAdditionalInstallFiles})
    file(TO_CMAKE_PATH "$ENV{olsAdditionalInstallFiles}" _ADDITIONAL_FILES)
  else()
    set(_ADDITIONAL_FILES "${CMAKE_SOURCE_DIR}/additional_install_files")
  endif()

  set(ENV{OLS_AdditionalInstallFiles} "${_ADDITIONAL_FILES}")
  unset(_ADDITIONAL_FILES)

  list(APPEND CMAKE_INCLUDE_PATH "$ENV{OLS_AdditionalInstallFiles}/include${_ARCH_SUFFIX}"
       "$ENV{OLS_AdditionalInstallFiles}/include")

  list(
    APPEND
    CMAKE_LIBRARY_PATH
    "$ENV{OLS_AdditionalInstallFiles}/lib${_ARCH_SUFFIX}"
    "$ENV{OLS_AdditionalInstallFiles}/lib"
    "$ENV{OLS_AdditionalInstallFiles}/libs${_ARCH_SUFFIX}"
    "$ENV{OLS_AdditionalInstallFiles}/libs"
    "$ENV{OLS_AdditionalInstallFiles}/bin${_ARCH_SUFFIX}"
    "$ENV{OLS_AdditionalInstallFiles}/bin")

  if(BUILD_FOR_DISTRIBUTION OR DEFINED ENV{CI})
    set_option(ENABLE_RTMPS ON)
  endif()
endmacro()
