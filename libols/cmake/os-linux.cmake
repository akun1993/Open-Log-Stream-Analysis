if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
target_compile_definitions(libols PRIVATE ENABLE_DARRAY_TYPE_TEST)
endif()

find_package(LibUUID REQUIRED)

target_sources(
libols
PRIVATE ols-nix.c
        ols-nix.h
        util/threading-posix.c
        util/threading-posix.h
        util/pipe-posix.c
        util/platform-nix.c)

target_link_libraries(libols PRIVATE  LibUUID::LibUUID)

if(OS_FREEBSD)
find_package(Sysinfo REQUIRED)
target_link_libraries(libobs PRIVATE Sysinfo::Sysinfo)
endif()

