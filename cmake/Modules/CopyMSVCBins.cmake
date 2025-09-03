# Doesn't really make sense anywhere else
if(NOT MSVC)
  return()
endif()

# Internal variable to avoid copying more than once
if(COPIED_DEPENDENCIES)
  return()
endif()

option(COPY_DEPENDENCIES "Automatically try copying all dependencies" ON)
if(NOT COPY_DEPENDENCIES)
  return()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_bin_suffix 64)
else()
  set(_bin_suffix 32)
endif()



file(
  GLOB
  SSL_BIN_FILES
  "${SSL_INCLUDE_DIR}/../bin${_bin_suffix}/ssleay32*.dll"
  "${SSL_INCLUDE_DIR}/../bin${_bin_suffix}/libeay32*.dll"
  "${SSL_INCLUDE_DIR}/../bin/ssleay32*.dll"
  "${SSL_INCLUDE_DIR}/../bin/libeay32*.dll"
  "${SSL_INCLUDE_DIR}/bin${_bin_suffix}/ssleay32*.dll"
  "${SSL_INCLUDE_DIR}/bin${_bin_suffix}/libeay32*.dll"
  "${SSL_INCLUDE_DIR}/bin/ssleay32*.dll"
  "${SSL_INCLUDE_DIR}/bin/libeay32*.dll")



file(
  GLOB
  LUA_BIN_FILES
  "${LUAJIT_INCLUDE_DIR}/../../bin${_bin_suffix}/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/../../bin/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/../bin${_bin_suffix}/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/../bin/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/bin${_bin_suffix}/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/bin/lua*.dll"
  "${LUAJIT_INCLUDE_DIR}/lua*.dll")

if(7Z_EXECUTABLE)
  get_filename_component(7Z_BIN_DIR ${7Z_EXECUTABLE} PATH)
endif()
  
file(
  GLOB
  7Z_BIN_FILES
  "${7Z_BIN_DIR}/../../bin${_bin_suffix}/7z.dll"
  "${7Z_BIN_DIR}/../../bin${_bin_suffix}/7z.exe"
  "${7Z_BIN_DIR}/../../bin/7z.exe"
  "${7Z_BIN_DIR}/../../bin/7z.dll"
  "${7Z_BIN_DIR}/../bin${_bin_suffix}/7z.exe"
  "${7Z_BIN_DIR}/../bin${_bin_suffix}/7z.dll"  
  "${7Z_BIN_DIR}/../bin/7z.exe"
  "${7Z_BIN_DIR}/../bin/7z.dll"  
  "${7Z_BIN_DIR}/bin${_bin_suffix}/7z.exe"
  "${7Z_BIN_DIR}/bin${_bin_suffix}/7z.dll"  
  "${7Z_BIN_DIR}/bin/7z.exe"
  "${7Z_BIN_DIR}/bin/7z.dll"  
  "${7Z_BIN_DIR}/7z.exe"
  "${7Z_BIN_DIR}/7z.dll")

if(XSLTPROC_EXECUTABLE)
  get_filename_component(XSLTPROC_BIN_DIR ${XSLTPROC_EXECUTABLE} PATH)
endif()
  
file(
  GLOB
  XSLTPROC_BIN_FILES
  "${XSLTPROC_BIN_DIR}/../../bin${_bin_suffix}/xsltproc.exe"
  "${XSLTPROC_BIN_DIR}/../../bin${_bin_suffix}/libxslt*.dll"
  "${XSLTPROC_BIN_DIR}/../../bin${_bin_suffix}/libxml2*.dll"  
  "${XSLTPROC_BIN_DIR}/../../bin${_bin_suffix}/libexslt*.dll"   
  "${XSLTPROC_BIN_DIR}/../../bin/xsltproc.exe"
  "${XSLTPROC_BIN_DIR}/../../bin/libxslt*.dll"
  "${XSLTPROC_BIN_DIR}/../../bin/libxml2*.dll"  
  "${XSLTPROC_BIN_DIR}/../../bin/libexslt*.dll" 
  "${XSLTPROC_BIN_DIR}/../bin${_bin_suffix}/xsltproc.exe"
  "${XSLTPROC_BIN_DIR}/../bin${_bin_suffix}/libxslt*.dll"
  "${XSLTPROC_BIN_DIR}/../bin${_bin_suffix}/libxml2*.dll"  
  "${XSLTPROC_BIN_DIR}/../bin${_bin_suffix}/libexslt*.dll" 
  "${XSLTPROC_BIN_DIR}/../bin/xsltproc.exe"
  "${XSLTPROC_BIN_DIR}/../bin/libxslt*.dll"
  "${XSLTPROC_BIN_DIR}/../bin/libxml2*.dll"  
  "${XSLTPROC_BIN_DIR}/../bin/libexslt*.dll" 
  "${XSLTPROC_BIN_DIR}/bin${_bin_suffix}/xsltproc.exe"
  "${XSLTPROC_BIN_DIR}/bin${_bin_suffix}/libxslt*.dll"
  "${XSLTPROC_BIN_DIR}/bin${_bin_suffix}/libxml2*.dll"  
  "${XSLTPROC_BIN_DIR}/bin${_bin_suffix}/libexslt*.dll" 
  "${XSLTPROC_BIN_DIR}/bin/xsltproc.exe"
  "${XSLTPROC_BIN_DIR}/bin/libxslt*.dll"
  "${XSLTPROC_BIN_DIR}/bin/libxml2*.dll"  
  "${XSLTPROC_BIN_DIR}/bin/libexslt*.dll" 
  "${XSLTPROC_BIN_DIR}/xsltproc.exe"
  "${XSLTPROC_BIN_DIR}/libxslt*.dll"
  "${XSLTPROC_BIN_DIR}/libxml2*.dll"  
  "${XSLTPROC_BIN_DIR}/libexslt*.dll")

if(ZLIB_LIB)
  get_filename_component(ZLIB_BIN_PATH ${ZLIB_LIB} PATH)
endif()
file(GLOB ZLIB_BIN_FILES "${ZLIB_BIN_PATH}/zlib*.dll")

if(NOT ZLIB_BIN_FILES)
  file(GLOB ZLIB_BIN_FILES "${ZLIB_INCLUDE_DIR}/../bin${_bin_suffix}/zlib*.dll" "${ZLIB_INCLUDE_DIR}/../bin/zlib*.dll"
       "${ZLIB_INCLUDE_DIR}/bin${_bin_suffix}/zlib*.dll" "${ZLIB_INCLUDE_DIR}/bin/zlib*.dll")
endif()

set(QtCore_DIR "${Qt6Core_DIR}")
cmake_path(SET QtCore_DIR_NORM NORMALIZE "${QtCore_DIR}/../../..")
set(QtCore_BIN_DIR "${QtCore_DIR_NORM}bin")
set(QtCore_PLUGIN_DIR "${QtCore_DIR_NORM}plugins")
ols_status(STATUS "QtCore_BIN_DIR: ${QtCore_BIN_DIR}")
ols_status(STATUS "QtCore_PLUGIN_DIR: ${QtCore_PLUGIN_DIR}")

file(
  GLOB
  QT_DEBUG_BIN_FILES
  "${QtCore_BIN_DIR}/Qt6Cored.dll"
  "${QtCore_BIN_DIR}/Qt6Guid.dll"
  "${QtCore_BIN_DIR}/Qt6Widgetsd.dll"
  "${QtCore_BIN_DIR}/Qt6Svgd.dll"
  "${QtCore_BIN_DIR}/Qt6Xmld.dll"
  "${QtCore_BIN_DIR}/Qt6Networkd.dll")
file(GLOB QT_DEBUG_PLAT_BIN_FILES "${QtCore_PLUGIN_DIR}/platforms/qwindowsd.dll")
file(GLOB QT_DEBUG_STYLES_BIN_FILES "${QtCore_PLUGIN_DIR}/styles/qwindowsvistastyled.dll")
file(GLOB QT_DEBUG_ICONENGINE_BIN_FILES "${QtCore_PLUGIN_DIR}/iconengines/qsvgicond.dll")
file(GLOB QT_DEBUG_IMAGEFORMATS_BIN_FILES "${QtCore_PLUGIN_DIR}/imageformats/qsvgd.dll"
     "${QtCore_PLUGIN_DIR}/imageformats/qgifd.dll" "${QtCore_PLUGIN_DIR}/imageformats/qjpegd.dll")

file(
  GLOB
  QT_BIN_FILES
  "${QtCore_BIN_DIR}/Qt6Core.dll"
  "${QtCore_BIN_DIR}/Qt6Gui.dll"
  "${QtCore_BIN_DIR}/Qt6Widgets.dll"
  "${QtCore_BIN_DIR}/Qt6Svg.dll"
  "${QtCore_BIN_DIR}/Qt6Xml.dll"
  "${QtCore_BIN_DIR}/Qt6Network.dll")
file(GLOB QT_PLAT_BIN_FILES "${QtCore_PLUGIN_DIR}/platforms/qwindows.dll")
file(GLOB QT_STYLES_BIN_FILES "${QtCore_PLUGIN_DIR}/styles/qwindowsvistastyle.dll")
file(GLOB QT_ICONENGINE_BIN_FILES "${QtCore_PLUGIN_DIR}/iconengines/qsvgicon.dll")
file(GLOB QT_IMAGEFORMATS_BIN_FILES "${QtCore_PLUGIN_DIR}/imageformats/qsvg.dll"
     "${QtCore_PLUGIN_DIR}/imageformats/qgif.dll" "${QtCore_PLUGIN_DIR}/imageformats/qjpeg.dll")

file(GLOB QT_ICU_BIN_FILES "${QtCore_BIN_DIR}/icu*.dll")

set(ALL_BASE_BIN_FILES
    ${LUA_BIN_FILES}
    ${XSLTPROC_BIN_FILES}
    ${7Z_BIN_FILES}
    ${SSL_BIN_FILES}
    ${ZLIB_BIN_FILES}
    ${QT_ICU_BIN_FILES})

set(ALL_REL_BIN_FILES ${QT_BIN_FILES})

set(ALL_DBG_BIN_FILES ${QT_DEBUG_BIN_FILES})

set(ALL_PLATFORM_BIN_FILES)
set(ALL_PLATFORM_REL_BIN_FILES ${QT_PLAT_BIN_FILES})
set(ALL_PLATFORM_DBG_BIN_FILES ${QT_DEBUG_PLAT_BIN_FILES})

set(ALL_STYLES_BIN_FILES)
set(ALL_STYLES_REL_BIN_FILES ${QT_STYLES_BIN_FILES})
set(ALL_STYLES_DBG_BIN_FILES ${QT_DEBUG_STYLES_BIN_FILES})

set(ALL_ICONENGINE_BIN_FILES)
set(ALL_ICONENGINE_REL_BIN_FILES ${QT_ICONENGINE_BIN_FILES})
set(ALL_ICONENGINE_DBG_BIN_FILES ${QT_DEBUG_ICONENGINE_BIN_FILES})

set(ALL_IMAGEFORMATS_BIN_FILES)
set(ALL_IMAGEFORMATS_REL_BIN_FILES ${QT_IMAGEFORMATS_BIN_FILES})
set(ALL_IMAGEFORMATS_DBG_BIN_FILES ${QT_DEBUG_IMAGEFORMATS_BIN_FILES})

foreach(
  list
  ALL_BASE_BIN_FILES
  ALL_REL_BIN_FILES
  ALL_DBG_BIN_FILES
  ALL_PLATFORM_BIN_FILES
  ALL_PLATFORM_REL_BIN_FILES
  ALL_PLATFORM_DBG_BIN_FILES
  ALL_STYLES_BIN_FILES
  ALL_STYLES_REL_BIN_FILES
  ALL_STYLES_DBG_BIN_FILES
  ALL_ICONENGINE_BIN_FILES
  ALL_ICONENGINE_REL_BIN_FILES
  ALL_ICONENGINE_DGB_BIN_FILES
  ALL_IMAGEFORMATS_BIN_FILES
  ALL_IMAGEFORMATS_REL_BIN_FILES
  ALL_IMAGEFORMATS_DGB_BIN_FILES)
  if(${list})
    list(REMOVE_DUPLICATES ${list})
  endif()
endforeach()

ols_status(STATUS "xsltproc files: ${XSLTPROC_BIN_FILES}")
ols_status(STATUS "7z files: ${7Z_BIN_FILES}")
ols_status(STATUS "lua files: ${LUA_BIN_FILES}")
ols_status(STATUS "ssl files: ${SSL_BIN_FILES}")
ols_status(STATUS "zlib files: ${ZLIB_BIN_FILES}")
ols_status(STATUS "Qt Debug files: ${QT_DEBUG_BIN_FILES}")
ols_status(STATUS "Qt Debug Platform files: ${QT_DEBUG_PLAT_BIN_FILES}")
ols_status(STATUS "Qt Debug Styles files: ${QT_DEBUG_STYLES_BIN_FILES}")
ols_status(STATUS "Qt Debug Iconengine files: ${QT_DEBUG_ICONENGINE_BIN_FILES}")
ols_status(STATUS "Qt Debug Imageformat files: ${QT_DEBUG_IMAGEFORMATS_BIN_FILES}")
ols_status(STATUS "Qt Release files: ${QT_BIN_FILES}")
ols_status(STATUS "Qt Release Platform files: ${QT_PLAT_BIN_FILES}")
ols_status(STATUS "Qt Release Styles files: ${QT_STYLES_BIN_FILES}")
ols_status(STATUS "Qt Release Iconengine files: ${QT_ICONENGINE_BIN_FILES}")
ols_status(STATUS "Qt Release Imageformat files: ${QT_IMAGEFORMATS_BIN_FILES}")
ols_status(STATUS "Qt ICU files: ${QT_ICU_BIN_FILES}")

foreach(BinFile ${ALL_BASE_BIN_FILES})
  ols_status(STATUS "copying ${BinFile} to ${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/")
endforeach()

foreach(BinFile ${ALL_REL_BIN_FILES})
  ols_status(STATUS "copying ${BinFile} to ${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/")
endforeach()

foreach(BinFile ${ALL_DBG_BIN_FILES})
  ols_status(STATUS "copying ${BinFile} to ${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/")
endforeach()

foreach(BinFile ${ALL_PLATFORM_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/platforms")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/platforms/")
endforeach()

foreach(BinFile ${ALL_PLATFORM_REL_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/platforms")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/platforms/")
endforeach()

foreach(BinFile ${ALL_PLATFORM_DBG_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/platforms")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/platforms/")
endforeach()

foreach(BinFile ${ALL_STYLES_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/styles")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/styles/")
endforeach()

foreach(BinFile ${ALL_STYLES_REL_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/styles")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/styles/")
endforeach()

foreach(BinFile ${ALL_STYLES_DBG_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/styles")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/styles/")
endforeach()

foreach(BinFile ${ALL_ICONENGINE_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/iconengines")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/iconengines/")
endforeach()

foreach(BinFile ${ALL_ICONENGINE_REL_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/iconengines")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/iconengines/")
endforeach()

foreach(BinFile ${ALL_ICONENGINE_DBG_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/iconengines")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/iconengines/")
endforeach()

foreach(BinFile ${ALL_IMAGEFORMATS_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/imageformats")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}/imageformats/")
endforeach()

foreach(BinFile ${ALL_IMAGEFORMATS_REL_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/imageformats")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}r/imageformats/")
endforeach()

foreach(BinFile ${ALL_IMAGEFORMATS_DBG_BIN_FILES})
  make_directory("${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/imageformats")
  file(COPY "${BinFile}" DESTINATION "${CMAKE_SOURCE_DIR}/additional_install_files/exec${_bin_suffix}d/imageformats/")
endforeach()

set(COPIED_DEPENDENCIES
    TRUE
    CACHE BOOL "Dependencies have been copied, set to false to copy again" FORCE)
