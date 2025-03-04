cmake_minimum_required(VERSION 3.16...3.25)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)" OR OLS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

  project(log-analysis VERSION ${OLS_VERSION_CANONICAL})

  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/32bit/projects.cmake")
    return()
  endif()

  include(compilerconfig)
  include(defaults)
  include(helpers)

  option(ENABLE_UI "Enable building with UI (requires Qt)" ON)
  option(ENABLE_SCRIPTING "Enable scripting support" ON)

  add_subdirectory(libols)

#  add_subdirectory(plugins)

#  add_subdirectory(UI)

  message_configuration()
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
message("Version Config In")
include(VersionConfig)
message("Version Config Out")

# Prohibit in-source builds
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  message(FATAL_ERROR "OLS: You cannot build in a source directory (or any directory with CMakeLists.txt file). "
                      "Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

project(log-analysis VERSION ${OLS_VERSION_CANONICAL})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Use target folders for MSVC/Xcode/etc.
include(DeprecationHelpers)
include(OlsHelpers)
# Set default compiler flags
include(CompilerConfig)

# Allow selection of common build types via UI
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING "OLS build type [Release, RelWithDebInfo, Debug, MinSizeRel]" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release RelWithDebInfo Debug MinSizeRel)
endif()

# Global project options

option(BUILD_FOR_DISTRIBUTION "Build for distribution (enables optimizations)" OFF)
option(ENABLE_UI "Enable building with UI (requires Qt)" ON)
option(ENABLE_SCRIPTING "Enable scripting support" ON)
option(USE_LIBCXX "Use libc++ instead of libstdc++" ${APPLE})
option(BUILD_TESTS "Build test directory (includes test sources and possibly a platform test executable)" OFF)

if(OS_WINDOWS)
  option(INSTALLER_RUN
         "Build a multiarch installer (needs to run independently after both archs have compiled) (Windows)" OFF)

elseif(OS_POSIX)
  option(LINUX_PORTABLE "Build portable version (Linux)" OFF)
  option(USE_XDG "Utilize XDG Base Directory Specification (Linux)" ON)

  if(OS_LINUX)
    option(BUILD_FOR_PPA "Build for PPA distribution" OFF)
  endif()
endif()

setup_ols_project()
mark_as_advanced(BUILD_TESTS USE_LIBCXX)

if(INSTALLER_RUN)
  generate_multiarch_installer()
  return()
endif()

# OLS sources and plugins
add_subdirectory(deps)
message("lib ols in")
add_subdirectory(libols)
message("lib ols out")
add_subdirectory(plugins)
message("lib ols ${OLS_VERSION}")

# const char *OLS_VERSION = "@OLS_VERSION@";
# const char *OLS_VERSION_CANONICAL = "@OLS_VERSION_CANONICAL@";
# const char *OLS_COMMIT = "@OLS_COMMIT@";

message("lib ols ${OLS_VERSION_CANONICAL}")
message("lib ols ${OLS_COMMIT}")
# OLS main app
#add_subdirectory(UI)

# Tests
if(ENABLE_UNIT_TESTS)
  enable_testing()
endif()

if(BUILD_TESTS OR ENABLE_UNIT_TESTS)
  add_subdirectory(test)
endif()
