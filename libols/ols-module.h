/******************************************************************************
    Copyright (C) 2023 by Kun Zhang <akun1993@163.com>

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

#include "ols.h"

#ifdef __cplusplus
#define MODULE_EXPORT extern "C" EXPORT
#define MODULE_EXTERN extern "C"
#else
#define MODULE_EXPORT EXPORT
#define MODULE_EXTERN extern
#endif

/**
 * @file
 * @brief This file is used by modules for module declaration and module
 *        exports.
 *
 * @page modules_page Modules
 * @brief Modules or plugins are libraries that can be loaded by libols and
 *        subsequently interact with it.
 *
 * @section modules_overview_sec Overview
 *
 * Modules can provide a wide range of functionality to libols, they for example
 * can feed captured audio or video to libols, or interface with an encoder to
 * provide some codec to libols.
 *
 * @section modules_basic_sec Creating a basic module
 *
 * In order to create a module for libols you will need to build a shared
 * library that implements a basic interface for libols to interact with.
 * The following code would create a simple source plugin without localization:
 *
@code
#include <ols-module.h>

OLS_DECLARE_MODULE()

extern struct ols_source_info my_source;

bool ols_module_load(void)
{
	ols_register_source(&my_source);
	return true;
}
@endcode
 *
 * If you want to enable localization, you will need to also use the
 * @ref OLS_MODULE_USE_DEFAULT_LOCALE() macro.
 *
 * Other module types:
 * - @ref ols_register_source()
 * - @ref ols_register_service()
 * - @ref ols_register_output()
 *
 */

/** Required: Declares a libols module. */
#define OLS_DECLARE_MODULE()                                             \
	static ols_module_t *ols_module_pointer;                         \
	MODULE_EXPORT void ols_module_set_pointer(ols_module_t *module); \
	void ols_module_set_pointer(ols_module_t *module)                \
	{                                                                \
		ols_module_pointer = module;                             \
	}                                                                \
	ols_module_t *ols_current_module(void)                           \
	{                                                                \
		return ols_module_pointer;                               \
	}                                                                \
	MODULE_EXPORT uint32_t ols_module_ver(void);                     \
	uint32_t ols_module_ver(void)                                    \
	{                                                                \
		return LIBOLS_API_VER;                                   \
	}

/**
 * Required: Called when the module is loaded.  Use this function to load all
 * the sources/encoders/outputs/services for your module, or anything else that
 * may need loading.
 *
 * @return           Return true to continue loading the module, otherwise
 *                   false to indicate failure and unload the module
 */
MODULE_EXPORT bool ols_module_load(void);

/** Optional: Called when the module is unloaded.  */
MODULE_EXPORT void ols_module_unload(void);

/** Optional: Called when all modules have finished loading */
MODULE_EXPORT void ols_module_post_load(void);

/** Called to set the current locale data for the module.  */
MODULE_EXPORT void ols_module_set_locale(const char *locale);

/** Called to free the current locale data for the module.  */
MODULE_EXPORT void ols_module_free_locale(void);

/** Optional: Use this macro in a module to use default locale handling. */
#define OLS_MODULE_USE_DEFAULT_LOCALE(module_name, default_locale)      \
	lookup_t *ols_module_lookup = NULL;                             \
	const char *ols_module_text(const char *val)                    \
	{                                                               \
		const char *out = val;                                  \
		text_lookup_getstr(ols_module_lookup, val, &out);       \
		return out;                                             \
	}                                                               \
	bool ols_module_get_string(const char *val, const char **out)   \
	{                                                               \
		return text_lookup_getstr(ols_module_lookup, val, out); \
	}                                                               \
	void ols_module_set_locale(const char *locale)                  \
	{                                                               \
		if (ols_module_lookup)                                  \
			text_lookup_destroy(ols_module_lookup);         \
		ols_module_lookup = ols_module_load_locale(             \
			ols_current_module(), default_locale, locale);  \
	}                                                               \
	void ols_module_free_locale(void)                               \
	{                                                               \
		text_lookup_destroy(ols_module_lookup);                 \
		ols_module_lookup = NULL;                               \
	}

/** Helper function for looking up locale if default locale handler was used */
MODULE_EXTERN const char *ols_module_text(const char *lookup_string);

/** Helper function for looking up locale if default locale handler was used,
 * returns true if text found, otherwise false */
MODULE_EXPORT bool ols_module_get_string(const char *lookup_string,
					 const char **translated_string);

/** Helper function that returns the current module */
MODULE_EXTERN ols_module_t *ols_current_module(void);

/**
 * Returns the location to a module data file associated with the current
 * module.  Free with bfree when complete.  Equivalent to:
 *    ols_find_module_file(ols_current_module(), file);
 */
#define ols_module_file(file) ols_find_module_file(ols_current_module(), file)

/**
 * Returns the location to a module config file associated with the current
 * module.  Free with bfree when complete.  Will return NULL if configuration
 * directory is not set.  Equivalent to:
 *    ols_module_get_config_path(ols_current_module(), file);
 */
#define ols_module_config_path(file) \
	ols_module_get_config_path(ols_current_module(), file)

/**
 * Optional: Declares the author(s) of the module
 *
 * @param name Author name(s)
 */
#define OLS_MODULE_AUTHOR(name)                            \
	MODULE_EXPORT const char *ols_module_author(void); \
	const char *ols_module_author(void)                \
	{                                                  \
		return name;                               \
	}

/** Optional: Returns the full name of the module */
MODULE_EXPORT const char *ols_module_name(void);

/** Optional: Returns a description of the module */
MODULE_EXPORT const char *ols_module_description(void);
