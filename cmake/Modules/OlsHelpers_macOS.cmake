# Helper function to set up runtime or library targets
function(setup_binary_target target)
  set_target_properties(
    ${target}
    PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.olsproject.${target}"
               XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist")

  set(MACOSX_PLUGIN_BUNDLE_NAME
      "${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_GUI_IDENTIFIER
      "com.olsproject.${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_BUNDLE_VERSION
      "${MACOSX_BUNDLE_BUNDLE_VERSION}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING
      "${MACOSX_BUNDLE_SHORT_VERSION_STRING}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_EXECUTABLE_NAME
      "${target}"
      PARENT_SCOPE)

  if(${target} STREQUAL libols)
    setup_framework_target(${target})
    set_property(GLOBAL APPEND PROPERTY OLS_FRAMEWORK_LIST "${target}")
  endif()
endfunction()

# Helper function to set-up framework targets on macOS
function(setup_framework_target target)
  set_target_properties(
    ${target}
    PROPERTIES FRAMEWORK ON
               FRAMEWORK_VERSION A
               OUTPUT_NAME "${target}"
               MACOSX_FRAMEWORK_IDENTIFIER "com.olsproject.${target}"
               MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/Plugin-Info.plist.in"
               XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.olsproject.${target}")

  install(
    TARGETS ${target}
    EXPORT "${target}Targets"
    FRAMEWORK DESTINATION "Frameworks"
              COMPONENT ols_libraries
              EXCLUDE_FROM_ALL
    INCLUDES
    DESTINATION Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers
    PUBLIC_HEADER
      DESTINATION Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers
      COMPONENT ols_libraries
      EXCLUDE_FROM_ALL)
endfunction()

# Helper function to set up OLS plugin targets
function(setup_plugin_target target)
  set(MACOSX_PLUGIN_BUNDLE_NAME
      "${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_GUI_IDENTIFIER
      "com.olsproject.${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_BUNDLE_VERSION
      "${MACOSX_BUNDLE_BUNDLE_VERSION}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_SHORT_VERSION_STRING
      "${MACOSX_BUNDLE_SHORT_VERSION_STRING}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_EXECUTABLE_NAME
      "${target}"
      PARENT_SCOPE)
  set(MACOSX_PLUGIN_BUNDLE_TYPE
      "BNDL"
      PARENT_SCOPE)

  set_target_properties(
    ${target}
    PROPERTIES BUNDLE ON
               BUNDLE_EXTENSION "plugin"
               OUTPUT_NAME "${target}"
               MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/Plugin-Info.plist.in"
               XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.olsproject.${target}"
               XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist")

  set_property(GLOBAL APPEND PROPERTY OLS_MODULE_LIST "${target}")
  ols_status(ENABLED "${target}")

  install_bundle_resources(${target})
endfunction()

# Helper function to set up OLS scripting plugin targets
function(setup_script_plugin_target target)
  set_target_properties(
    ${target}
    PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.olsproject.${target}"
               XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist")

  set_property(GLOBAL APPEND PROPERTY OLS_SCRIPTING_MODULE_LIST "${target}")
  ols_status(ENABLED "${target}")
endfunction()

# Helper function to set up target resources (e.g. L10N files)
function(setup_target_resources target destination)
  install_bundle_resources(${target})
endfunction()

# Helper function to set up plugin resources inside plugin bundle
function(install_bundle_resources target)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    file(GLOB_RECURSE _DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
    foreach(_DATA_FILE IN LISTS _DATA_FILES)
      file(RELATIVE_PATH _RELATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/data/ ${_DATA_FILE})
      get_filename_component(_RELATIVE_PATH "${_RELATIVE_PATH}" PATH)
      target_sources(${target} PRIVATE ${_DATA_FILE})
      set_source_files_properties(${_DATA_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${_RELATIVE_PATH}")
      string(REPLACE "\\" "\\\\" _GROUP_NAME "${_RELATIVE_PATH}")
      source_group("Resources\\${_GROUP_NAME}" FILES ${_DATA_FILE})
    endforeach()
  endif()
endfunction()

# Helper function to set up specific resource files for targets
function(add_target_resource target resource destination)
  target_sources(${target} PRIVATE ${resource})
  set_source_files_properties(${resource} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endfunction()

# Helper function to set up OLS app target
function(setup_ols_app target)
  set_target_properties(
    ${target}
    PROPERTIES BUILD_WITH_INSTALL_RPATH OFF
               XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/entitlements.plist"
               XCODE_SCHEME_ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")

  install(TARGETS ${target} BUNDLE DESTINATION "." COMPONENT ols_app)

  setup_ols_frameworks(${target})
  setup_ols_modules(${target})
  setup_ols_bundle(${target})
endfunction()


# Helper function to set-up OLS frameworks for macOS bundling
function(setup_ols_frameworks target)
  get_property(OLS_FRAMEWORK_LIST GLOBAL PROPERTY OLS_FRAMEWORK_LIST)
  install(
    TARGETS ${OLS_FRAMEWORK_LIST}
    RUNTIME DESTINATION "$<TARGET_FILE_BASE_NAME:${target}>.app/Contents/Frameworks/" COMPONENT ols_frameworks
    LIBRARY DESTINATION "$<TARGET_FILE_BASE_NAME:${target}>.app/Contents/Frameworks/" COMPONENT ols_frameworks
    FRAMEWORK DESTINATION "$<TARGET_FILE_BASE_NAME:${target}>.app/Contents/Frameworks/" COMPONENT ols_frameworks
    PUBLIC_HEADER
      DESTINATION "${OLS_INCLUDE_DESTINATION}"
      COMPONENT ols_libraries
      EXCLUDE_FROM_ALL)
endfunction()

# Helper function to set-up OLS plugins and helper binaries for macOS bundling
function(setup_ols_modules target)

  get_property(OLS_MODULE_LIST GLOBAL PROPERTY OLS_MODULE_LIST)
  list(LENGTH OLS_MODULE_LIST _LEN)
  if(_LEN GREATER 0)
    add_dependencies(${target} ${OLS_MODULE_LIST})

    install(
      TARGETS ${OLS_MODULE_LIST}
      LIBRARY DESTINATION "PlugIns"
              COMPONENT ols_plugin_dev
              EXCLUDE_FROM_ALL)

    install(
      TARGETS ${OLS_MODULE_LIST}
      LIBRARY DESTINATION $<TARGET_FILE_BASE_NAME:${target}>.app/Contents/PlugIns
              COMPONENT ols_plugins
              NAMELINK_COMPONENT ${target}_Development)
  endif()

  get_property(OLS_SCRIPTING_MODULE_LIST GLOBAL PROPERTY OLS_SCRIPTING_MODULE_LIST)
  list(LENGTH OLS_SCRIPTING_MODULE_LIST _LEN)
  if(_LEN GREATER 0)
    add_dependencies(${target} ${OLS_SCRIPTING_MODULE_LIST})

    install(
      TARGETS ${OLS_SCRIPTING_MODULE_LIST}
      LIBRARY DESTINATION "PlugIns"
              COMPONENT ols_plugin_dev
              EXCLUDE_FROM_ALL)

    if(TARGET olspython)
      install(
        FILES "$<TARGET_FILE_DIR:olspython>/olspython.py"
        DESTINATION "Resources"
        COMPONENT ols_plugin_dev
        EXCLUDE_FROM_ALL)
    endif()

    install(TARGETS ${OLS_SCRIPTING_MODULE_LIST} LIBRARY DESTINATION $<TARGET_FILE_BASE_NAME:ols>.app/Contents/PlugIns
                                                         COMPONENT ols_scripting_plugins)
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" --install .. --config $<CONFIG> --prefix $<TARGET_BUNDLE_CONTENT_DIR:${target}>
            --component ols_plugin_dev > /dev/null
    COMMENT "Installing OLS plugins for development"
    VERBATIM)
endfunction()

# Helper function to finalize macOS app bundles
function(setup_ols_bundle target)
  install(
    CODE "
    set(_DEPENDENCY_PREFIX \"${CMAKE_PREFIX_PATH}\")
    set(_BUILD_FOR_DISTRIBUTION \"${BUILD_FOR_DISTRIBUTION}\")
    set(_BUNDLENAME \"$<TARGET_FILE_BASE_NAME:${target}>.app\")
    set(_BUNDLER_COMMAND \"${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/dylibbundler\")
    set(_CODESIGN_IDENTITY \"${OLS_BUNDLE_CODESIGN_IDENTITY}\")
    set(_CODESIGN_ENTITLEMENTS \"${CMAKE_SOURCE_DIR}/cmake/bundle/macOS\")"
    COMPONENT ols_resources)

  if(SPARKLE_APPCAST_URL AND SPARKLE_PUBLIC_KEY)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        /bin/sh -c
        "plutil -replace SUFeedURL -string ${SPARKLE_APPCAST_URL} \"$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Info.plist\""
      VERBATIM)

    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        /bin/sh -c
        "plutil -replace SUPublicEDKey -string \"${SPARKLE_PUBLIC_KEY}\" \"$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Info.plist\""
      VERBATIM)

    install(
      DIRECTORY ${SPARKLE}
      DESTINATION $<TARGET_FILE_BASE_NAME:${target}>.app/Contents/Frameworks
      USE_SOURCE_PERMISSIONS
      COMPONENT ols_frameworks)
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND
      /usr/bin/sed -i '' 's/font-size: 10pt\;/font-size: 12pt\;/'
      "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/themes/Acri.qss"
      "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/themes/Grey.qss"
      "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/themes/Light.qss"
      "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/themes/Rachni.qss"
      "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/themes/Yami.qss")

  install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/bundleutils.cmake" COMPONENT ols_resources)
endfunction()

# Helper function to export target to build and install tree Allows usage of `find_package(libols)` by other build trees
function(export_target target)
  get_target_property(_IS_FRAMEWORK ${target} FRAMEWORK)

  set(OLS_PLUGIN_DESTINATION "")
  set(OLS_DATA_DESTINATION "")

  if(_IS_FRAMEWORK)
    export_framework_target(${target})
  else()
    _export_target(${ARGV})
  endif()
  set_target_properties(${target} PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h")
endfunction()

# Helper function to export macOS framework targets
function(export_framework_target)
  set(CMAKE_EXPORT_PACKAGE_REGISTRY OFF)

  include(GenerateExportHeader)
  generate_export_header(${target} EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h")

  target_sources(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${target}_EXPORT.h")

  set(TARGETS_EXPORT_NAME "${target}Targets")
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${target}Config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake"
    INSTALL_DESTINATION Frameworks/${target}.framework/Resources/cmake
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
    DESTINATION Frameworks/${target}.framework/Resources/cmake
    COMPONENT ols_libraries
    EXCLUDE_FROM_ALL)

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target}Config.cmake ${CMAKE_CURRENT_BINARY_DIR}/${target}ConfigVersion.cmake
    DESTINATION Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Resources/cmake
    COMPONENT ols_libraries
    EXCLUDE_FROM_ALL)
endfunction()

# Helper function to install header files
function(install_headers target)
  install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
    DESTINATION
      $<IF:$<BOOL:$<TARGET_PROPERTY:${target},FRAMEWORK>>,Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers,${OLS_INCLUDE_DESTINATION}>
    COMPONENT ols_libraries
    EXCLUDE_FROM_ALL FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "util/windows" EXCLUDE
    PATTERN "cmake" EXCLUDE
    PATTERN "pkgconfig" EXCLUDE
    PATTERN "data" EXCLUDE)

  install(
    FILES "${CMAKE_BINARY_DIR}/config/olsconfig.h"
    DESTINATION
      $<IF:$<BOOL:$<TARGET_PROPERTY:${target},FRAMEWORK>>,Frameworks/$<TARGET_FILE_BASE_NAME:${target}>.framework/Headers,${OLS_INCLUDE_DESTINATION}>
    COMPONENT ols_libraries
    EXCLUDE_FROM_ALL)
endfunction()
