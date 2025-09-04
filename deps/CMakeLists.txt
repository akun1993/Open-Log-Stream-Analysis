if(OS_WINDOWS)
  if(NOT MINGW)
    add_subdirectory(w32-pthreads)
  endif()
  add_subdirectory(ipc-util)
endif()

add_subdirectory(opts-parser)
add_subdirectory(uthash-header)

# Use bundled jansson version as fallback
find_package(Jansson 2.5 QUIET)
if(NOT TARGET Jansson::Jansson)
  ols_status(STATUS "Jansson >=2.5 not found, building bundled version.")

  add_subdirectory(jansson)
  #set(JANSSON_LIBRARIES ${Jansson_LIB})
  add_library(Jansson::Jansson ALIAS jansson)
else()
  ols_status(STATUS "Using system Jansson library.")
endif()

add_subdirectory(tinyxml2)

add_subdirectory(ols-scripting)

add_subdirectory(pcre2)

if(OS_WINDOWS)
  #add_subdirectory(libxml2)
  #add_subdirectory(libxslt)  
else()
  find_program(
    XSLTPROC_EXECUTABLE
    NAMES xsltproc
    HINTS ENV )

  if(NOT XSLTPROC_EXECUTABLE)
    ols_status(STATUS "Using system Jansson library.")
    find_package(LibXml2 CONFIG QUIET)
    if(NOT TARGET LibXml2::LibXml2)
      add_subdirectory(libxml2)
    else()
      ols_status(STATUS "Using system LibXml2 library.")
    endif()
    
    add_subdirectory(libxslt)  
  endif() # NOT XSLTPROC_EXECUTABLE

endif()



