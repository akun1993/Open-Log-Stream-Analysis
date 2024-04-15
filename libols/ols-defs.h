/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@olsproject.com>

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

/** Maximum number of source channels for output and per display */
#define MAX_CHANNELS 64


#define MODULE_SUCCESS 0
#define MODULE_ERROR -1
#define MODULE_FILE_NOT_FOUND -2
#define MODULE_MISSING_EXPORTS -3
#define MODULE_INCOMPATIBLE_VER -4
#define MODULE_HARDCODED_SKIP -5

#define OLS_OUTPUT_SUCCESS 0
#define OLS_OUTPUT_BAD_PATH -1
#define OLS_OUTPUT_CONNECT_FAILED -2
#define OLS_OUTPUT_INVALID_STREAM -3
#define OLS_OUTPUT_ERROR -4
#define OLS_OUTPUT_DISCONNECTED -5
#define OLS_OUTPUT_UNSUPPORTED -6
#define OLS_OUTPUT_NO_SPACE -7


