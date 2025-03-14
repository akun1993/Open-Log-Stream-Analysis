
# cmake-format: off
# cmake-format: on
find_package(Sysinfo REQUIRED)

target_sources(
  libols
  PRIVATE # cmake-format: sortable
          util/pipe-posix.c
          util/platform-nix.c
          util/threading-posix.c
          util/threading-posix.h)

target_compile_definitions(libols PRIVATE $<$<COMPILE_LANG_AND_ID:C,GNU>:ENABLE_DARRAY_TYPE_TEST>
                                          $<$<COMPILE_LANG_AND_ID:CXX,GNU>:ENABLE_DARRAY_TYPE_TEST>)



set_target_properties(libols PROPERTIES OUTPUT_NAME ols)
