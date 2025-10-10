option(WITH_QCANPOOL "Build qcanpool library" ON)


if (WITH_QCANPOOL)
  add_subdirectory(qcanpool)
endif()


