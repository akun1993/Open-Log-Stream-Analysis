cmake_minimum_required(VERSION 3.20)

# Enable modern cmake policies
if(POLICY CMP0009)
  cmake_policy(SET CMP0009 NEW)
endif()

if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif()

if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()

# Build options
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES
      "${CMAKE_HOST_SYSTEM_PROCESSOR}"
      CACHE STRING "OLS build architecture for macOS - x86_64 required at least" FORCE)
endif()
set_property(CACHE CMAKE_OSX_ARCHITECTURES PROPERTY STRINGS arm64 x86_64 "x86_64;arm64")

if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=arm64] "11.0")
  set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET[arch=x86_64] "10.15")

  if("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64")
    set(_MACOS_DEPLOYMENT_TARGET "11.0")
  else()
    set(_MACOS_DEPLOYMENT_TARGET "10.15")
  endif()

  set(CMAKE_OSX_DEPLOYMENT_TARGET
      "${_MACOS_DEPLOYMENT_TARGET}"
      CACHE STRING "OLS deployment target for macOS - 10.15+ required" FORCE)
  unset(_MACOS_DEPLOYMENT_TARGET)
endif()
set_property(CACHE CMAKE_OSX_DEPLOYMENT_TARGET PROPERTY STRINGS 10.15 11.0 12.0 13.0)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      "${CMAKE_BINARY_DIR}/install"
      CACHE STRING "Directory to install OLS after building" FORCE)
endif()

if(NOT DEFINED CMAKE_PREFIX_PATH)
  message(
    WARNING "No CMAKE_PREFIX_PATH set: OLS supplies pre-built dependencies for building on macOS.\n"
            "While OLS can be built using packages installed via Homebrew, pre-built dependencies "
            "contain beneficial patches and fixes for use within OLS and is the suggested source "
            "of these dependencies.\n"
            "You can download the appropriate ols-deps package for your "
            "architecture and set CMAKE_PREFIX_PATH to this directory:\n"
            "CMAKE_PREFIX_PATH=\"<PATH_TO_OLS_DEPS>\"\n"
            "Download pre-built OLS dependencies at https://github.com/olsproject/ols-deps/releases\n")
endif()

# SWIG hard codes the directory to its library directory at compile time. As ols-deps need to be relocatable, we need to
# force SWIG to look for its files in a directory relative to the PREFIX_PATH. The best way to ensure this is to set the
# SWIG_LIB environment variable.

if(NOT DEFINED ENV{SWIG_LIB} AND EXISTS "${CMAKE_PREFIX_PATH}/bin/swig")
  set(ENV{SWIG_LIB} "${CMAKE_PREFIX_PATH}/share/swig/CURRENT")
endif()

macro(setup_ols_project)
  set(CMAKE_XCODE_GENERATE_SCHEME ON)

  # Set code signing options
  if(NOT OLS_BUNDLE_CODESIGN_TEAM)
    set(OLS_BUNDLE_CODESIGN_TEAM
        ""
        CACHE STRING "OLS code signing team for macOS" FORCE)
    if(NOT OLS_BUNDLE_CODESIGN_IDENTITY)
      set(OLS_BUNDLE_CODESIGN_IDENTITY
          "-"
          CACHE STRING "OLS code signing identity for macOS" FORCE)
    endif()
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${OLS_BUNDLE_CODESIGN_IDENTITY}")
  else()
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic")
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${OLS_BUNDLE_CODESIGN_TEAM}")
  endif()

  set(OLS_CODESIGN_ENTITLEMENTS
      "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist"
      CACHE INTERNAL "Path to codesign entitlements plist")
  set(OLS_CODESIGN_LINKER
      ON
      CACHE BOOL "Enable linker code-signing on macOS (macOS 11+ required)")

  # Tell Xcode to pretend the linker signed binaries so that editing with install_name_tool preserves ad-hoc signatures.
  # This option is supported by codesign on macOS 11 or higher. See CMake Issue 21854:
  # https://gitlab.kitware.com/cmake/cmake/-/issues/21854

  if(OLS_CODESIGN_LINKER)
    set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "-o linker-signed")
  endif()

  # Set default options for bundling on macOS
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_SKIP_BUILD_RPATH OFF)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
  set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks/")
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)

  # Set bundle parameters for cmake's automatic plist generation
  set(MACOSX_BUNDLE_EXECUTABLE_NAME "OLS")
  set(MACOSX_BUNDLE_BUNDLE_NAME "${OLS_PRODUCT_NAME}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${OLS_BUILD_NUMBER}")
  set(MACOSX_BUNDLE_COPYRIGHT "${OLS_LEGAL_COPYRIGHT}")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.olsproject.ols-studio")
  set(MACOSX_BUNDLE_ICON_FILE "AppIcon")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OLS_VERSION_CANONICAL}")
  string(TIMESTAMP CURRENT_YEAR "%Y")

  # Set paths for distribution bundling
  set(OLS_BUNDLE_NAME "OLS")
  set(OLS_EXECUTABLE_DESTINATION "${CMAKE_INSTALL_BINDIR}")
  set(OLS_INCLUDE_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ols")
  set(OLS_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  set(OLS_CMAKE_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake")

  if(BUILD_FOR_DISTRIBUTION)
    set_option(CMAKE_BUILD_TYPE "Release")
    set(CPACK_PACKAGE_VERSION "${OLS_VERSION_CANONICAL}")
  else()
    set(CPACK_PACKAGE_VERSION "${OLS_VERSION_CANONICAL}-${OLS_BUILD_NUMBER}")
  endif()

  if(BUILD_FOR_DISTRIBUTION OR DEFINED ENV{CI})
    set_option(ENABLE_RTMPS ON)
  endif()

  set(CPACK_PACKAGE_NAME "OLS")
  set(CPACK_PACKAGE_VENDOR "${OLS_WEBSITE}")
  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${OLS_COMMENTS}")

  if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
    set(CPACK_ARCH_SUFFIX "Intel")
  elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    set(CPACK_ARCH_SUFFIX "Apple")
  else()
    set(CPACK_ARCH_SUFFIX "Universal")
  endif()

  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macOS-${CPACK_ARCH_SUFFIX}")

  set(CPACK_COMPONENTS_ALL ols_app ols_frameworks ols_plugins ols_scripting_plugins ols_resources)
  set(CPACK_COMPONENT_OLS_APP_DISPLAY_NAME "OLS Studio")
  set(CPACK_COMPONENT_OLS_FRAMEWORKS_DISPLAY_NAME "OLS Frameworks")
  set(CPACK_COMPONENT_OLS_PLUGINS_DISPLAY_NAME "OLS Plugins")
  set(CPACK_COMPONENT_OLS_SCRIPTING_PLUGINS_DISPLAY_NAME "OLS Scripting Plugins")
  set(CPACK_COMPONENT_OLS_RESOURCES_DISPLAY_NAME "OLS Resources")

  set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/background.tiff")
  set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/AppIcon.icns")
  get_filename_component(CPACK_DMG_BACKGROUND_FILENAME ${CPACK_DMG_BACKGROUND_IMAGE} NAME)
  set(CPACK_DMG_FORMAT "UDZO")
  set(CPACK_DMG_FILESYSTEM "APFS")
  set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_BINARY_DIR}/package.applescript")

  set(_DMG_WINDOW_X "100")
  set(_DMG_WINDOW_Y "100")
  set(_DMG_WINDOW_WIDTH "540")
  set(_DMG_WINDOW_HEIGHT "380")
  set(_DMG_ICON_SIZE "96")
  set(_DMG_TEXT_SIZE "16")
  set(_DMG_OLS_X "124")
  set(_DMG_OLS_Y "180")
  set(_DMG_APP_LINK_X "416")
  set(_DMG_APP_LINK_Y "180")

  configure_file("${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/package.applescript.in"
                 "${CMAKE_BINARY_DIR}/package.applescript" @ONLY)

  include(CPack)

  if(ENABLE_UI)
    install(
      CODE "
  		set(_BUNDLENAME \"$<TARGET_FILE_BASE_NAME:ols>.app\")
  		if(EXISTS \"\${CMAKE_INSTALL_PREFIX}/\${_BUNDLENAME}\")
  			file(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}/\${_BUNDLENAME}\")
  		endif()")
  endif()
endmacro()
