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

#include <ols-data.h>
#include <ols-properties.h>
#include <stdarg.h>
#include <util/c99defs.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ols_script;
typedef struct ols_script ols_script_t;

enum ols_script_lang {
  OLS_SCRIPT_LANG_UNKNOWN,
  OLS_SCRIPT_LANG_LUA,
  OLS_SCRIPT_LANG_PYTHON
};

EXPORT bool ols_scripting_load(void);
EXPORT void ols_scripting_unload(void);
EXPORT const char **ols_scripting_supported_formats(void);

EXPORT void ols_scripting_prase(ols_script_t *script, const char *data,
                                int len);

typedef void (*scripting_log_handler_t)(void *p, ols_script_t *script, int lvl,
                                        const char *msg);

EXPORT void ols_scripting_set_log_callback(scripting_log_handler_t handler,
                                           void *param);

EXPORT bool ols_scripting_python_runtime_linked(void);
EXPORT void ols_scripting_python_version(char *version, size_t version_length);
EXPORT bool ols_scripting_python_loaded(void);
EXPORT bool ols_scripting_load_python(const char *python_path);

EXPORT ols_script_t *ols_script_create(const char *path, ols_data_t *settings);
EXPORT void ols_script_destroy(ols_script_t *script);

EXPORT const char *ols_script_get_description(const ols_script_t *script);
EXPORT const char *ols_script_get_path(const ols_script_t *script);
EXPORT const char *ols_script_get_file(const ols_script_t *script);
EXPORT enum ols_script_lang ols_script_get_lang(const ols_script_t *script);

EXPORT bool ols_script_loaded(const ols_script_t *script);
EXPORT bool ols_script_reload(ols_script_t *script);

#ifdef __cplusplus
}
#endif
