# Enable modern cmake policies
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif()

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

if(POLICY CMP0095)
  cmake_policy(SET CMP0095 NEW)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND LINUX_PORTABLE)
  set(CMAKE_INSTALL_PREFIX
      "${CMAKE_BINARY_DIR}/install"
      CACHE STRING "Directory to install OLS after building" FORCE)
endif()

macro(setup_ols_project)
  #[[
	POSIX directory setup (portable)
	CMAKE_BINARY_DIR
		└ rundir
			└ CONFIG
				└ bin
					└ ARCH
				└ data
					└ libols
					└ ols-plugins
						└ PLUGIN
					└ ols-scripting
						└ ARCH
					└ ols-studio
				└ ols-plugins
					└ ARCH

	POSIX directory setup (non-portable)
	/usr/local/
		└ bin
		└ include
			└ ols
		└ libs
			└ cmake
			└ ols-plugins
			└ ols-scripting
		└ share
			└ ols
				└ libols
				└ ols-plugins
				└ ols-studio
	#]]

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_ARCH_SUFFIX 64)
  else()
    set(_ARCH_SUFFIX 32)
  endif()

  if(NOT OLS_MULTIARCH_SUFFIX AND DEFINED ENV{OLS_MULTIARCH_SUFFIX})
    set(OLS_MULTIARCH_SUFFIX "$ENV{OLS_MULTIARCH_SUFFIX}")
  endif()

  set(OLS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/rundir")

  if(NOT LINUX_PORTABLE)
    set(OLS_EXECUTABLE_DESTINATION "${CMAKE_INSTALL_BINDIR}")
    set(OLS_INCLUDE_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ols")
    set(OLS_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    set(OLS_PLUGIN_DESTINATION "${OLS_LIBRARY_DESTINATION}/ols-plugins")
    set(OLS_PLUGIN_PATH "${OLS_PLUGIN_DESTINATION}")
    set(OLS_SCRIPT_PLUGIN_DESTINATION "${OLS_LIBRARY_DESTINATION}/ols-scripting")
    set(OLS_DATA_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/ols")
    set(OLS_CMAKE_DESTINATION "${OLS_LIBRARY_DESTINATION}/cmake")
    set(OLS_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/")
    set(OLS_DATA_PATH "${OLS_DATA_DESTINATION}")
    message("not portable")
    set(OLS_SCRIPT_PLUGIN_PATH "${CMAKE_INSTALL_PREFIX}/${OLS_SCRIPT_PLUGIN_DESTINATION}")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${OLS_LIBRARY_DESTINATION}")
  else()
    set(OLS_EXECUTABLE_DESTINATION "bin/${_ARCH_SUFFIX}bit")
    set(OLS_INCLUDE_DESTINATION "include")
    set(OLS_LIBRARY_DESTINATION "bin/${_ARCH_SUFFIX}bit")
    set(OLS_PLUGIN_DESTINATION "ols-plugins/${_ARCH_SUFFIX}bit")
    set(OLS_PLUGIN_PATH "../../${OLS_PLUGIN_DESTINATION}")
    set(OLS_SCRIPT_PLUGIN_DESTINATION "data/ols-scripting/${_ARCH_SUFFIX}bit")
    set(OLS_DATA_DESTINATION "data")
    set(OLS_CMAKE_DESTINATION "cmake")

    set(OLS_INSTALL_PREFIX "")
    set(OLS_DATA_PATH "../../${OLS_DATA_DESTINATION}")

    set(OLS_SCRIPT_PLUGIN_PATH "../../${OLS_SCRIPT_PLUGIN_DESTINATION}")
    set(CMAKE_INSTALL_RPATH "$ORIGIN/" "$ORIGIN/../../${OLS_LIBRARY_DESTINATION}")
  endif()


  if(BUILD_FOR_DISTRIBUTION OR DEFINED ENV{CI})
    set_option(ENABLE_RTMPS ON)
  endif()

  set(CPACK_PACKAGE_NAME "ols-studio")
  set(CPACK_PACKAGE_VENDOR "${OLS_WEBSITE}")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${OLS_COMPANY_NAME}")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${OLS_COMMENTS}")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/UI/data/license/gplv2.txt")
  set(CPACK_PACKAGE_VERSION "${OLS_VERSION_CANONICAL}-${OLS_BUILD_NUMBER}")
  set(CPACK_PACKAGE_EXECUTABLES "ols")

  if(OS_LINUX AND NOT LINUX_PORTABLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_SET_DESTDIR ON)
    set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)
  elseif(OS_FREEBSD)
    option(ENABLE_CPACK_GENERATOR "Enable FreeBSD CPack generator (experimental)" OFF)

    if(ENABLE_CPACK_GENERATOR)
      set(CPACK_GENERATOR "FreeBSD")
    endif()

    set(CPACK_FREEBSD_PACKAGE_DEPS
        "devel/cmake"
        "devel/jansson"
        "devel/libsysinfo"
        "devel/ninja"
        "devel/pkgconf"
        "devel/qt5-buildtools"
        "devel/qt5-core"
        "devel/qt5-qmake"
        "lang/lua52"
        "lang/luajit"
        "lang/python37"
        "security/mbedtls"
        "textproc/qt5-xml")
  endif()
  include(CPack)
endmacro()
