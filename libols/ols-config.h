/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

/*
 * LIBOLS_API_VER is returned by module_version in each module.
 *
 * Libols uses semantic versioning.  See http://semver.org/ for more
 * information.
 */

/*
 * Increment if major breaking API changes
 */
#define LIBOLS_API_MAJOR_VER 30

/*
 * Increment if backward-compatible additions
 *
 * Reset to zero each major version
 */
#define LIBOLS_API_MINOR_VER 1

/*
 * Increment if backward-compatible bug fix
 *
 * Reset to zero each major or minor version
 */
#define LIBOLS_API_PATCH_VER 1

#define MAKE_SEMANTIC_VERSION(major, minor, patch)                             \
  ((major << 24) | (minor << 16) | patch)

#define LIBOLS_API_VER                                                         \
  MAKE_SEMANTIC_VERSION(LIBOLS_API_MAJOR_VER, LIBOLS_API_MINOR_VER,            \
                        LIBOLS_API_PATCH_VER)

#ifdef HAVE_OLSCONFIG_H
#include "olsconfig.h"
#else
#define OLS_VERSION "unknown"
#define OLS_DATA_PATH "../../data"
#define OLS_INSTALL_PREFIX ""
#define OLS_PLUGIN_DESTINATION "ols-plugins"
#define OLS_RELATIVE_PREFIX "../../"
#define OLS_RELEASE_CANDIDATE 0
#define OLS_BETA 0
#endif

#define OLS_INSTALL_DATA_PATH OLS_INSTALL_PREFIX OLS_DATA_PATH
