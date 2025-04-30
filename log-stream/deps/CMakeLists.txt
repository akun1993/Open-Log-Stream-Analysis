if(OS_WINDOWS)
  if(NOT MINGW)
    add_subdirectory(w32-pthreads)
  endif()
  add_subdirectory(ipc-util)
endif()

add_subdirectory(opts-parser)
add_subdirectory(uthash)

# Use bundled jansson version as fallback
find_package(Jansson 2.5 QUIET)
if(NOT TARGET Jansson::Jansson)
  ols_status(STATUS "Jansson >=2.5 not found, building bundled version.")

  add_subdirectory(jansson)
  add_library(Jansson::Jansson ALIAS jansson)
else()
  ols_status(STATUS "Using system Jansson library.")
endif()

