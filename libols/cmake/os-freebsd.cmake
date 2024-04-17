
# cmake-format: off
# cmake-format: on
find_package(Sysinfo REQUIRED)

target_sources(
  libobs
  PRIVATE # cmake-format: sortable
          util/pipe-posix.c
          util/platform-nix.c
          util/threading-posix.c
          util/threading-posix.h)

target_compile_definitions(libobs PRIVATE $<$<COMPILE_LANG_AND_ID:C,GNU>:ENABLE_DARRAY_TYPE_TEST>
                                          $<$<COMPILE_LANG_AND_ID:CXX,GNU>:ENABLE_DARRAY_TYPE_TEST>)


if(TARGET gio::gio)
  target_sources(libobs PRIVATE util/platform-nix-dbus.c util/platform-nix-portal.c)
  target_link_libraries(libobs PRIVATE gio::gio)
endif()


set_target_properties(libobs PROPERTIES OUTPUT_NAME obs)
