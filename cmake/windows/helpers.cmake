# OLS CMake Windows helper functions module

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=R0912
# cmake-lint: disable=R0915
# cmake-format: on

include_guard(GLOBAL)

include(helpers_common)

# set_target_properties_ols: Set target properties for use in log-analysis
function(set_target_properties_ols target)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 0 _STPO "${options}" "${oneValueArgs}" "${multiValueArgs}")

  message(DEBUG "Setting additional properties for target ${target}...")

  while(_STPO_PROPERTIES)
    list(POP_FRONT _STPO_PROPERTIES key value)
    set_property(TARGET ${target} PROPERTY ${key} "${value}")
  endwhile()

  get_target_property(target_type ${target} TYPE)

  if(target_type STREQUAL EXECUTABLE)
    # cmake-format: off
    _target_install_ols(${target} DESTINATION ${OLS_EXECUTABLE_DESTINATION})
    # cmake-format: on

    if(target STREQUAL log-analysis)
      get_property(ols_executables GLOBAL PROPERTY _OLS_EXECUTABLES)
      get_property(ols_modules GLOBAL PROPERTY OLS_MODULES_ENABLED)
      add_dependencies(${target} ${ols_executables} ${ols_modules})
      _bundle_dependencies(${target})
      target_add_resource(${target} "${CMAKE_CURRENT_SOURCE_DIR}/../AUTHORS"
                          "${OLS_DATA_DESTINATION}/log-analysis/authors")
    else()
      set_property(GLOBAL APPEND PROPERTY _OLS_EXECUTABLES ${target})
    endif()
  elseif(target_type STREQUAL SHARED_LIBRARY)
    set_target_properties(${target} PROPERTIES VERSION ${OLS_VERSION_MAJOR} SOVERSION ${OLS_VERSION_CANONICAL})

    # cmake-format: off
    _target_install_ols(
      ${target}
        DESTINATION "${OLS_EXECUTABLE_DESTINATION}"
        LIBRARY_DESTINATION "${OLS_LIBRARY_DESTINATION}"
        HEADER_DESTINATION "${OLS_INCLUDE_DESTINATION}")
    # cmake-format: on
  elseif(target_type STREQUAL MODULE_LIBRARY)
    set_target_properties(${target} PROPERTIES VERSION 0 SOVERSION ${OLS_VERSION_CANONICAL})

    if(target STREQUAL "olspython" OR target STREQUAL "olslua")
      set(target_destination "${OLS_SCRIPT_PLUGIN_DESTINATION}")
    else()
      set(target_destination "${OLS_PLUGIN_DESTINATION}")
    endif()

    # cmake-format: off
    _target_install_ols(${target} DESTINATION ${target_destination})
    # cmake-format: on

    if(${target} STREQUAL olspython)
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E echo "Add olspython import module"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_SCRIPT_PLUGIN_DESTINATION}/"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE_DIR:olspython>/olspython.py"
                "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_SCRIPT_PLUGIN_DESTINATION}/"
        COMMENT "")

      install(
        FILES "$<TARGET_FILE_DIR:olspython>/olspython.py"
        DESTINATION "${OLS_SCRIPT_PLUGIN_DESTINATION}"
        COMPONENT Runtime)
    endif()

    set_property(GLOBAL APPEND PROPERTY OLS_MODULES_ENABLED ${target})
  endif()

  target_link_options(${target} PRIVATE "/PDBALTPATH:$<TARGET_PDB_FILE_NAME:${target}>")
  target_install_resources(${target})

  get_target_property(target_sources ${target} SOURCES)
  set(target_ui_files ${target_sources})
  list(FILTER target_ui_files INCLUDE REGEX ".+\\.(ui|qrc)")
  source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "UI Files"
    FILES ${target_ui_files})

  if(${target} STREQUAL libols)
    set(target_source_files ${target_sources})
    set(target_header_files ${target_sources})
    list(FILTER target_source_files INCLUDE REGEX ".+\\.(m|c[cp]?p?|swift)")
    list(FILTER target_header_files INCLUDE REGEX ".+\\.h(pp)?")

    source_group(
      TREE "${CMAKE_CURRENT_SOURCE_DIR}"
      PREFIX "Source Files"
      FILES ${target_source_files})
    source_group(
      TREE "${CMAKE_CURRENT_SOURCE_DIR}"
      PREFIX "Header Files"
      FILES ${target_header_files})
  endif()
endfunction()

# _target_install_ols: Helper function to install build artifacts to rundir and install location
function(_target_install_ols target)
  set(options "32BIT")
  set(oneValueArgs "DESTINATION" "LIBRARY_DESTINATION" "HEADER_DESTINATION")
  set(multiValueArgs "")
  cmake_parse_arguments(PARSE_ARGV 0 _TIO "${options}" "${oneValueArgs}" "${multiValueArgs}")

  if(_TIO_32BIT)
    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL EXECUTABLE)
      set(suffix exe)
    else()
      set(suffix dll)
    endif()

    cmake_path(RELATIVE_PATH CMAKE_CURRENT_SOURCE_DIR BASE_DIRECTORY "${OLS_SOURCE_DIR}" OUTPUT_VARIABLE project_path)

    set(32bit_project_path "${OLS_SOURCE_DIR}/build_x86/${project_path}")
    set(target_file "${32bit_project_path}/$<CONFIG>/${target}32.${suffix}")
    set(target_pdb_file "${32bit_project_path}/$<CONFIG>/${target}32.pdb")
    set(comment "Copy ${target} (32-bit) to destination")

    install(
      FILES "${32bit_project_path}/$<CONFIG>/${target}32.${suffix}"
      DESTINATION "${_TIO_DESTINATION}"
      COMPONENT Runtime
      OPTIONAL)
  else()
    set(target_file "$<TARGET_FILE:${target}>")
    set(target_pdb_file "$<TARGET_PDB_FILE:${target}>")
    set(comment "Copy ${target} to destination")

    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL EXECUTABLE)
      install(TARGETS ${target} RUNTIME DESTINATION "${_TIO_DESTINATION}" COMPONENT Runtime)
    elseif(target_type STREQUAL SHARED_LIBRARY)
      if(NOT _TIO_LIBRARY_DESTINATION)
        set(_TIO_LIBRARY_DESTINATION ${_TIO_DESTINATION})
      endif()
      if(NOT _TIO_HEADER_DESTINATION)
        set(_TIO_HEADER_DESTINATION include)
      endif()
      install(
        TARGETS ${target}
        RUNTIME DESTINATION "${_TIO_DESTINATION}"
        LIBRARY DESTINATION "${_TIO_LIBRARY_DESTINATION}"
                COMPONENT Runtime
                EXCLUDE_FROM_ALL
        PUBLIC_HEADER
          DESTINATION "${_TIO_HEADER_DESTINATION}"
          COMPONENT Development
          EXCLUDE_FROM_ALL)
    elseif(target_type STREQUAL MODULE_LIBRARY)
      install(
        TARGETS ${target}
        LIBRARY DESTINATION "${_TIO_DESTINATION}"
                COMPONENT Runtime
                NAMELINK_COMPONENT Development)
    endif()
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "${comment}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OLS_OUTPUT_DIR}/$<CONFIG>/${_TIO_DESTINATION}"
    COMMAND "${CMAKE_COMMAND}" -E copy ${target_file} "${OLS_OUTPUT_DIR}/$<CONFIG>/${_TIO_DESTINATION}"
    COMMAND "${CMAKE_COMMAND}" -E $<IF:$<CONFIG:Debug,RelWithDebInfo,Release>,copy,true> ${target_pdb_file}
            "${OLS_OUTPUT_DIR}/$<CONFIG>/${_TIO_DESTINATION}"
    COMMENT ""
    VERBATIM)

  install(
    FILES ${target_pdb_file}
    CONFIGURATIONS RelWithDebInfo Debug Release
    DESTINATION "${_TIO_DESTINATION}"
    COMPONENT Runtime
    OPTIONAL)
endfunction()

# target_export: Helper function to export target as CMake package
function(target_export target)
  # Exclude CMake package from 'ALL' target
  set(exclude_variant EXCLUDE_FROM_ALL)
  _target_export(${target})

  get_target_property(target_type ${target} TYPE)
  if(NOT target_type STREQUAL INTERFACE_LIBRARY)
    install(
      FILES "$<TARGET_PDB_FILE:${target}>"
      CONFIGURATIONS RelWithDebInfo Debug Release
      DESTINATION "${OLS_EXECUTABLE_DESTINATION}"
      COMPONENT Development
      OPTIONAL)
  endif()
endfunction()

# Helper function to add resources into bundle
function(target_install_resources target)
  message(DEBUG "Installing resources for target ${target}...")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    file(GLOB_RECURSE data_files "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
    foreach(data_file IN LISTS data_files)
      cmake_path(RELATIVE_PATH data_file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/" OUTPUT_VARIABLE
                 relative_path)
      cmake_path(GET relative_path PARENT_PATH relative_path)
      target_sources(${target} PRIVATE "${data_file}")
      source_group("Resources/${relative_path}" FILES "${data_file}")
    endforeach()

    get_property(ols_module_list GLOBAL PROPERTY OLS_MODULES_ENABLED)
    if(target IN_LIST ols_module_list)
      set(target_destination "${OLS_DATA_DESTINATION}/ols-plugins/${target}")
    elseif(target STREQUAL log-analysis)
      set(target_destination "${OLS_DATA_DESTINATION}/log-analysis")
    else()
      set(target_destination "${OLS_DATA_DESTINATION}/${target}")
    endif()

    install(
      DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/"
      DESTINATION "${target_destination}"
      USE_SOURCE_PERMISSIONS
      COMPONENT Runtime)

    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "Copy ${target} resources to data directory"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${OLS_OUTPUT_DIR}/$<CONFIG>/${target_destination}"
      COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data"
              "${OLS_OUTPUT_DIR}/$<CONFIG>/${target_destination}"
      COMMENT ""
      VERBATIM)
  endif()
endfunction()

# Helper function to add a specific resource to a bundle
function(target_add_resource target resource)
  get_property(ols_module_list GLOBAL PROPERTY OLS_MODULES_ENABLED)
  if(ARGN)
    set(target_destination "${ARGN}")
  elseif(${target} IN_LIST ols_module_list)
    set(target_destination "${OLS_DATA_DESTINATION}/ols-plugins/${target}")
  elseif(target STREQUAL log-analysis)
    set(target_destination "${OLS_DATA_DESTINATION}/log-analysis")
  else()
    set(target_destination "${OLS_DATA_DESTINATION}/${target}")
  endif()

  message(DEBUG "Add resource '${resource}' to target ${target} at destination '${target_destination}'...")

  install(
    FILES "${resource}"
    DESTINATION "${target_destination}"
    COMPONENT Runtime)

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Copy ${target} resource ${resource} to library directory"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OLS_OUTPUT_DIR}/$<CONFIG>/${target_destination}/"
    COMMAND "${CMAKE_COMMAND}" -E copy "${resource}" "${OLS_OUTPUT_DIR}/$<CONFIG>/${target_destination}/"
    COMMENT ""
    VERBATIM)

  source_group("Resources" FILES "${resource}")
endfunction()

# _bundle_dependencies: Resolve third party dependencies and add them to Windows binary directory
function(_bundle_dependencies target)
  message(DEBUG "Discover dependencies of target ${target}...")
  set(found_dependencies)
  find_dependencies(TARGET ${target} FOUND_VAR found_dependencies)

  get_property(ols_module_list GLOBAL PROPERTY OLS_MODULES_ENABLED)
  list(LENGTH ols_module_list num_modules)
  if(num_modules GREATER 0)
    add_dependencies(${target} ${ols_module_list})
    foreach(module IN LISTS ols_module_list)
      find_dependencies(TARGET ${module} FOUND_VAR found_dependencies)
    endforeach()
  endif()

  list(REMOVE_DUPLICATES found_dependencies)
  set(library_paths_DEBUG)
  set(library_paths_RELWITHDEBINFO)
  set(library_paths_RELEASE)
  set(library_paths_MINSIZEREL)
  set(plugins_list)

  foreach(library IN LISTS found_dependencies)

    get_target_property(library_type ${library} TYPE)
    get_target_property(is_imported ${library} IMPORTED)

    if(is_imported)
      get_target_property(imported_location ${library} IMPORTED_LOCATION)

      foreach(config IN ITEMS RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)
        get_target_property(imported_location_${config} ${library} IMPORTED_LOCATION_${config})
        if(imported_location_${config})
          _check_library_location(${imported_location_${config}})
        elseif(NOT imported_location_${config} AND imported_location_RELEASE)
          _check_library_location(${imported_location_RELEASE})
        else()
          _check_library_location(${imported_location})
        endif()
      endforeach()

      if(library MATCHES "Qt[56]?::.+")
        find_qt_plugins(COMPONENT ${library} TARGET ${target} FOUND_VAR plugins_list)
      endif()
    endif()
  endforeach()

  foreach(config IN ITEMS DEBUG RELWITHDEBINFO RELEASE MINSIZEREL)
    list(REMOVE_DUPLICATES library_paths_${config})
  endforeach()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Copy dependencies to binary directory (${OLS_EXECUTABLE_DESTINATION})..."
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_EXECUTABLE_DESTINATION}"
    COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Debug>,copy_if_different,true>"
            "$<$<CONFIG:Debug>:${library_paths_DEBUG}>" "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_EXECUTABLE_DESTINATION}"
    COMMAND
      "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:RelWithDebInfo>,copy_if_different,true>"
      "$<$<CONFIG:RelWithDebInfo>:${library_paths_RELWITHDEBINFO}>"
      "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_EXECUTABLE_DESTINATION}"
    COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Release>,copy_if_different,true>"
            "$<$<CONFIG:Release>:${library_paths_RELEASE}>" "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_EXECUTABLE_DESTINATION}"
    COMMAND
      "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:MinSizeRel>,copy_if_different,true>"
      "$<$<CONFIG:MinSizeRel>:${library_paths_MINSIZEREL}>" "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_EXECUTABLE_DESTINATION}"
    COMMENT "."
    VERBATIM COMMAND_EXPAND_LISTS)

  install(
    FILES ${library_paths_DEBUG}
    CONFIGURATIONS Debug
    DESTINATION "${OLS_EXECUTABLE_DESTINATION}"
    COMPONENT Runtime)

  install(
    FILES ${library_paths_RELWITHDEBINFO}
    CONFIGURATIONS RelWithDebInfo
    DESTINATION "${OLS_EXECUTABLE_DESTINATION}"
    COMPONENT Runtime)

  install(
    FILES ${library_paths_RELEASE}
    CONFIGURATIONS Release
    DESTINATION "${OLS_EXECUTABLE_DESTINATION}"
    COMPONENT Runtime)

  install(
    FILES ${library_paths_MINSIZEREL}
    CONFIGURATIONS MinSizeRel
    DESTINATION "${OLS_EXECUTABLE_DESTINATION}"
    COMPONENT Runtime)

  list(REMOVE_DUPLICATES plugins_list)
  foreach(plugin IN LISTS plugins_list)
    message(TRACE "Adding Qt plugin ${plugin}...")

    cmake_path(GET plugin PARENT_PATH plugin_path)
    cmake_path(GET plugin_path STEM plugin_stem)

    list(APPEND plugin_stems ${plugin_stem})

    if(plugin MATCHES ".+d\\.dll$")
      list(APPEND plugin_${plugin_stem}_debug ${plugin})
    else()
      list(APPEND plugin_${plugin_stem} ${plugin})
    endif()
  endforeach()

  list(REMOVE_DUPLICATES plugin_stems)
  foreach(stem IN LISTS plugin_stems)
    set(plugin_list ${plugin_${stem}})
    set(plugin_list_debug ${plugin_${stem}_debug})
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo
              "Copy Qt plugins ${stem} to binary directory (${OLS_EXECUTABLE_DESTINATION}/${stem})"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_EXECUTABLE_DESTINATION}/${stem}"
      COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Debug>,copy_if_different,true>" "${plugin_list_debug}"
              "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_EXECUTABLE_DESTINATION}/${stem}"
      COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Debug>,true,copy_if_different>" "${plugin_list}"
              "${OLS_OUTPUT_DIR}/$<CONFIG>/${OLS_EXECUTABLE_DESTINATION}/${stem}"
      COMMENT ""
      VERBATIM COMMAND_EXPAND_LISTS)

    install(
      FILES ${plugin_list_debug}
      CONFIGURATIONS Debug
      DESTINATION "${OLS_EXECUTABLE_DESTINATION}/${stem}"
      COMPONENT Runtime)

    install(
      FILES ${plugin_list}
      CONFIGURATIONS RelWithDebInfo Release MinSizeRel
      DESTINATION "${OLS_EXECUTABLE_DESTINATION}/${stem}"
      COMPONENT Runtime)
  endforeach()
endfunction()

# _check_library_location: Check for corresponding DLL given an import library path
macro(_check_library_location location)
  if(library_type STREQUAL "SHARED_LIBRARY")
    set(library_location "${location}")
  else()
    string(STRIP "${location}" location)
    if(location MATCHES ".+lib$")
      cmake_path(GET location FILENAME _dll_name)
      cmake_path(GET location PARENT_PATH _implib_path)
      cmake_path(SET _bin_path NORMALIZE "${_implib_path}/../bin")
      string(REPLACE ".lib" ".dll" _dll_name "${_dll_name}")
      string(REPLACE ".dll" ".pdb" _pdb_name "${_dll_name}")

      find_program(
        _dll_path
        NAMES "${_dll_name}"
        HINTS ${_implib_path} ${_bin_path} NO_CACHE
        NO_DEFAULT_PATH)

      find_program(
        _pdb_path
        NAMES "${_pdb_name}"
        HINTS ${_implib_path} ${_bin_path} NO_CACHE
        NO_DEFAULT_PATH)

      if(_dll_path)
        set(library_location "${_dll_path}")
        set(library_pdb_location "${_pdb_path}")
      else()
        unset(library_location)
        unset(library_pdb_location)
      endif()
      unset(_dll_path)
      unset(_pdb_path)
      unset(_bin_path)
      unset(_implib_path)
      unset(_dll_name)
      unset(_pdb_name)
    else()
      unset(library_location)
      unset(library_pdb_location)
    endif()
  endif()

  if(library_location)
    list(APPEND library_paths_${config} ${library_location})
  endif()
  if(library_pdb_location)
    list(APPEND library_paths_${config} ${library_pdb_location})
  endif()
  unset(location)
  unset(library_location)
  unset(library_pdb_location)
endmacro()
