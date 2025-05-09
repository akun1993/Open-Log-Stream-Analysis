# OLS CMake common CPack module

include_guard(GLOBAL)

# Set default global CPack variables
set(CPACK_PACKAGE_NAME log-analysis)
set(CPACK_PACKAGE_VENDOR "${OLS_WEBSITE}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${OLS_WEBSITE}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${OLS_COMMENTS}")
set(CPACK_PACKAGE_CHECKSUM SHA256)

set(CPACK_PACKAGE_VERSION_MAJOR ${OLS_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${OLS_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${OLS_VERSION_PATCH})
