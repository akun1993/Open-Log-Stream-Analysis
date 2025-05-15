set(MODULE_DESCRIPTION "OLS Library")
set(UI_VERSION "${OLS_VERSION_CANONICAL}")

configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ols-module.rc.in
               libols.rc)

target_sources(
  libols
  PRIVATE obs-win-crash-handler.c
          ols-windows.c
          util/threading-windows.c
          util/threading-windows.h
          util/pipe-windows.c
          util/platform-windows.c
          util/windows/device-enum.c
          util/windows/device-enum.h
          util/windows/obfuscate.c
          util/windows/obfuscate.h
          util/windows/win-registry.h
          util/windows/win-version.h
          util/windows/window-helpers.c
          util/windows/window-helpers.h
          util/windows/ComPtr.hpp
          util/windows/CoTaskMemPtr.hpp
          util/windows/HRError.hpp
          util/windows/WinHandle.hpp
          libobs.rc)

target_compile_definitions(
  libols PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS
                 _CRT_NONSTDC_NO_WARNINGS)

target_link_libraries(libols PRIVATE dxgi Avrt Dwmapi winmm)

if(MSVC)
  target_link_libraries(libols PUBLIC OBS::w32-pthreads)

  target_compile_options(libols PRIVATE "$<$<COMPILE_LANGUAGE:C>:/EHc->"
                                        "$<$<COMPILE_LANGUAGE:CXX>:/EHc->")

  target_link_options(libols PRIVATE "LINKER:/IGNORE:4098"
                      "LINKER:/SAFESEH:NO")
endif()