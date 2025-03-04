set(OLS_COMPANY_NAME "OLS Project")
set(OLS_PRODUCT_NAME "OLS Studio")
set(OLS_WEBSITE "https://www.olsproject.com")
set(OLS_COMMENTS "Free and open source software for log streaming analysis")
set(OLS_LEGAL_COPYRIGHT "(C) Lain Bailey")

# Configure default version strings
set(_OLS_DEFAULT_VERSION "0" "0" "1")
set(_OLS_RELEASE_CANDIDATE "0" "0" "0" "0")
set(_OLS_BETA "0" "0" "0" "0")

# Set full and canonical OLS version from current git tag or manual override
if(NOT DEFINED OLS_VERSION_OVERRIDE)
  if(NOT DEFINED RELEASE_CANDIDATE
     AND NOT DEFINED BETA
     AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git describe --always --tags --dirty=-modified
      OUTPUT_VARIABLE _OLS_VERSION
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE _OLS_VERSION_RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(_OLS_VERSION_RESULT EQUAL 0)
      if(${_OLS_VERSION} MATCHES "rc[0-9]+$")
        set(RELEASE_CANDIDATE ${_OLS_VERSION})
      elseif(${_OLS_VERSION} MATCHES "beta[0-9]+$")
        set(BETA ${_OLS_VERSION})
      else()
        string(REPLACE "-" "." _CANONICAL_SPLIT ${_OLS_VERSION})
        string(REPLACE "." ";" _CANONICAL_SPLIT ${_CANONICAL_SPLIT})
        list(GET _CANONICAL_SPLIT 0 1 2 _OLS_VERSION_CANONICAL)
        string(REPLACE "." ";" _OLS_VERSION ${_OLS_VERSION})
        # Get 8-character commit hash without "g" prefix
        foreach(VERSION_PART ${_CANONICAL_SPLIT})
          if(VERSION_PART MATCHES "^g")
            string(SUBSTRING ${VERSION_PART}, 1, 8, OLS_COMMIT)
            break()
          endif()
        endforeach()
      endif()
    endif()
  endif()

  # Set release candidate version information Must be a string in the format of "x.x.x-rcx"
  if(DEFINED RELEASE_CANDIDATE)
    string(REPLACE "-rc" "." _OLS_RELEASE_CANDIDATE ${RELEASE_CANDIDATE})
    string(REPLACE "." ";" _OLS_VERSION ${RELEASE_CANDIDATE})
    string(REPLACE "." ";" _OLS_RELEASE_CANDIDATE ${_OLS_RELEASE_CANDIDATE})
    list(GET _OLS_RELEASE_CANDIDATE 0 1 2 _OLS_VERSION_CANONICAL)
    # Set beta version information Must be a string in the format of "x.x.x-betax"
  elseif(DEFINED BETA)
    string(REPLACE "-beta" "." _OLS_BETA ${BETA})
    string(REPLACE "." ";" _OLS_VERSION ${BETA})
    string(REPLACE "." ";" _OLS_BETA ${_OLS_BETA})
    list(GET _OLS_BETA 0 1 2 _OLS_VERSION_CANONICAL)
  elseif(NOT DEFINED _OLS_VERSION)
    set(_OLS_VERSION ${_OLS_DEFAULT_VERSION})
    set(_OLS_VERSION_CANONICAL ${_OLS_DEFAULT_VERSION})
  endif()
else()
  string(REPLACE "." ";" _OLS_VERSION "${OLS_VERSION_OVERRIDE}")
  string(REPLACE "-" ";" _OLS_VERSION_CANONICAL "${OLS_VERSION_OVERRIDE}")
  list(GET _OLS_VERSION_CANONICAL 0 _OLS_VERSION_CANONICAL)
  string(REPLACE "." ";" _OLS_VERSION_CANONICAL "${_OLS_VERSION_CANONICAL}")
endif()

list(GET _OLS_VERSION_CANONICAL 0 OLS_VERSION_MAJOR)
list(GET _OLS_VERSION_CANONICAL 1 OLS_VERSION_MINOR)
list(GET _OLS_VERSION_CANONICAL 2 OLS_VERSION_PATCH)
list(GET _OLS_RELEASE_CANDIDATE 0 OLS_RELEASE_CANDIDATE_MAJOR)
list(GET _OLS_RELEASE_CANDIDATE 1 OLS_RELEASE_CANDIDATE_MINOR)
list(GET _OLS_RELEASE_CANDIDATE 2 OLS_RELEASE_CANDIDATE_PATCH)
list(GET _OLS_RELEASE_CANDIDATE 3 OLS_RELEASE_CANDIDATE)
list(GET _OLS_BETA 0 OLS_BETA_MAJOR)
list(GET _OLS_BETA 1 OLS_BETA_MINOR)
list(GET _OLS_BETA 2 OLS_BETA_PATCH)
list(GET _OLS_BETA 3 OLS_BETA)

string(REPLACE ";" "." OLS_VERSION_CANONICAL "${_OLS_VERSION_CANONICAL}")
string(REPLACE ";" "." OLS_VERSION "${_OLS_VERSION}")

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

# Define build number cache file
set(BUILD_NUMBER_CACHE
    ${CMAKE_SOURCE_DIR}/cmake/.CMakeBuildNumber
    CACHE INTERNAL "OLS build number cache file")

# Read build number from cache file or manual override
if(NOT DEFINED OLS_BUILD_NUMBER AND EXISTS ${BUILD_NUMBER_CACHE})
  file(READ ${BUILD_NUMBER_CACHE} OLS_BUILD_NUMBER)
  math(EXPR OLS_BUILD_NUMBER "${OLS_BUILD_NUMBER}+1")
elseif(NOT DEFINED OLS_BUILD_NUMBER)
  set(OLS_BUILD_NUMBER "1")
endif()
file(WRITE ${BUILD_NUMBER_CACHE} "${OLS_BUILD_NUMBER}")

message(STATUS "OLS:  Application Version: ${OLS_VERSION} - Build Number: ${OLS_BUILD_NUMBER}")
