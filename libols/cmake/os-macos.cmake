target_link_libraries(
  libols
  PRIVATE # cmake-format: sortable
          "$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,AudioToolbox.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,AudioUnit.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,Carbon.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreAudio.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,IOKit.framework>")

target_sources(
  libols
  PRIVATE # cmake-format: sortable
          ols-cocoa.m
          util/apple/cfstring-utils.h
          util/pipe-posix.c
          util/platform-cocoa.m
          util/platform-nix.c
          util/threading-posix.c
          util/threading-posix.h)

target_compile_options(libols PUBLIC -Wno-strict-prototypes -Wno-shorten-64-to-32)

set_property(SOURCE ols-cocoa.m util/platform-cocoa.m PROPERTY COMPILE_FLAGS -fobjc-arc)
set_property(TARGET libols PROPERTY FRAMEWORK TRUE)
