# OLS CMake common version helper module

include_guard(GLOBAL)

set(_ols_version ${_ols_default_version})
set(_ols_version_canonical ${_ols_default_version})

# Attempt to automatically discover expected OLS version
if(NOT DEFINED OLS_VERSION_OVERRIDE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git describe --always --tags --dirty=-modified
    OUTPUT_VARIABLE _ols_version
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE _ols_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(_ols_version_result EQUAL 0)
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1;\\2;\\3" _ols_version_canonical ${_ols_version})
  endif()
elseif(DEFINED OLS_VERSION_OVERRIDE)
  if(OLS_VERSION_OVERRIDE MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+).*")
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1;\\2;\\3" _ols_version_canonical
                         ${OLS_VERSION_OVERRIDE})
    set(_ols_version ${OLS_VERSION_OVERRIDE})
  else()
    message(FATAL_ERROR "Invalid version supplied - must be <MAJOR>.<MINOR>.<PATCH>[-(rc|beta)<NUMBER>].")
  endif()
endif()

# Set beta/rc versions if suffix included in version string
if(_ols_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+-rc[0-9]+")
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.[0-9]+-rc([0-9]+).*$" "\\1" _ols_release_candidate ${_ols_version})
elseif(_ols_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+-beta[0-9]+")
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.[0-9]+-beta([0-9]+).*$" "\\1" _ols_beta ${_ols_version})
endif()

list(GET _ols_version_canonical 0 OLS_VERSION_MAJOR)
list(GET _ols_version_canonical 1 OLS_VERSION_MINOR)
list(GET _ols_version_canonical 2 OLS_VERSION_PATCH)

set(OLS_RELEASE_CANDIDATE ${_ols_release_candidate})
set(OLS_BETA ${_ols_beta})

string(REPLACE ";" "." OLS_VERSION_CANONICAL "${_ols_version_canonical}")
string(REPLACE ";" "." OLS_VERSION "${_ols_version}")

if(OLS_RELEASE_CANDIDATE GREATER 0)
  message(
    AUTHOR_WARNING
      "******************************************************************************\n"
      "  + OLS-Studio - Release candidate detected, OLS_VERSION is now: ${OLS_VERSION}\n"
      "******************************************************************************")
elseif(OLS_BETA GREATER 0)
  message(
    AUTHOR_WARNING
      "******************************************************************************\n"
      "  + OLS-Studio - Beta detected, OLS_VERSION is now: ${OLS_VERSION}\n"
      "******************************************************************************")
endif()

unset(_ols_default_version)
unset(_ols_version)
unset(_ols_version_canonical)
unset(_ols_release_candidate)
unset(_ols_beta)
unset(_ols_version_result)
