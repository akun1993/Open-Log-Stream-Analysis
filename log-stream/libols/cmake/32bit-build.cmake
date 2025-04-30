if(OS_WINDOWS)

  add_library(ols-comutils INTERFACE)
  add_library(OLS::COMutils ALIAS ols-comutils)
  target_sources(ols-comutils INTERFACE util/windows/ComPtr.hpp)
  target_include_directories(ols-comutils INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

  add_library(ols-winhandle INTERFACE)
  add_library(OLS::winhandle ALIAS ols-winhandle)
  target_sources(ols-winhandle INTERFACE util/windows/WinHandle.hpp)
  target_include_directories(ols-winhandle INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
