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

#include <util/dstr.h>
#include <callback/calldata.h>
#include "ols-scripting.h"

struct ols_script {
	enum ols_script_lang type;
	bool loaded;

	ols_data_t *settings;

	struct dstr path;
	struct dstr file;
	struct dstr desc;
};

struct script_callback;
typedef void (*defer_call_cb)(void *param);

extern void defer_call_post(defer_call_cb call, void *cb);

extern void script_log(ols_script_t *script, int level, const char *format,
		       ...);
extern void script_log_va(ols_script_t *script, int level, const char *format,
			  va_list args);

#define script_error(script, format, ...) \
	script_log(script, LOG_ERROR, format, ##__VA_ARGS__)
#define script_warn(script, format, ...) \
	script_log(script, LOG_WARNING, format, ##__VA_ARGS__)
#define script_info(script, format, ...) \
	script_log(script, LOG_INFO, format, ##__VA_ARGS__)
#define script_debug(script, format, ...) \
	script_log(script, LOG_DEBUG, format, ##__VA_ARGS__)
