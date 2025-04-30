# OLS CMake bootstrap module

include_guard(GLOBAL)

# Map fallback configurations for optimized build configurations
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RelWithDebInfo Release MinSizeRel None "")
set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL MinSizeRel Release RelWithDebInfo None "")
set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE Release RelWithDebInfo MinSizeRel None "")

# Prohibit in-source builds
if("${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  message(FATAL_ERROR "In-source builds of OLS are not supported. "
                      "Specify a build directory via 'cmake -S <SOURCE DIRECTORY> -B <BUILD_DIRECTORY>' instead.")
  file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/CMakeCache.txt" "${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles")
endif()

# Use folders for source file organization with IDE generators (Visual Studio/Xcode)
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

# Set default global project variables
set(OLS_COMPANY_NAME "OLS Project")
set(OLS_PRODUCT_NAME "OLS Studio")
set(OLS_WEBSITE "https://www.olsproject.com")
set(OLS_COMMENTS "Free and open source software for video recording and live streaming")
set(OLS_LEGAL_COPYRIGHT "(C) Lain Bailey")
set(OLS_CMAKE_VERSION 3.0.0)

# Configure default version strings
set(_ols_default_version "0" "0" "1")
set(_ols_release_candidate 0)
set(_ols_beta 0)

# Add common module directories to default search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/finders")

include(policies NO_POLICY_SCOPE)
include(versionconfig)
include(buildnumber)
include(osconfig)

# Allow selection of common build types via UI
if(NOT CMAKE_GENERATOR MATCHES "(Xcode|Visual Studio .+)")
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        "RelWithDebInfo"
        CACHE STRING "OLS build type [Release, RelWithDebInfo, Debug, MinSizeRel]" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release RelWithDebInfo Debug MinSizeRel)
  endif()
endif()

# Enable default inclusion of targets' source and binary directory
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
