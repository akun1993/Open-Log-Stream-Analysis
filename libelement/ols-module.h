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

/* ------------------------------------------------------------------------- */
/* modules */
#include <stdbool.h>
#include <stdint.h>
#include "util/dstr.h"
#include "util/text-lookup.h"

typedef struct ols_module ols_module_t;

struct ols_module
{
	char *mod_name;
	const char *file;
	char *bin_path;
	char *data_path;
	void *module;
	bool loaded;

	bool (*load)(void);
	void (*unload)(void);
	void (*post_load)(void);
	void (*set_locale)(const char *locale);
	bool (*get_string)(const char *lookup_string, const char **translated_string);
	void (*free_locale)(void);
	uint32_t (*ver)(void);
	void (*set_pointer)(ols_module_t *module);
	const char *(*name)(void);
	const char *(*description)(void);
	const char *(*author)(void);

	struct ols_module *next;
};

extern void free_module(struct ols_module *mod);

struct ols_module_path
{
	char *bin;
	char *data;
};

static inline void free_module_path(struct ols_module_path *omp)
{
	if (omp)
	{
		bfree(omp->bin);
		bfree(omp->data);
	}
}

static inline bool check_path(const char *data, const char *path,
							  struct dstr *output)
{
	dstr_copy(output, path);
	dstr_cat(output, data);

	return os_file_exists(output->array);
}

/** Required: Declares a libols module. */
#define OLS_DECLARE_MODULE()                                         \
	static ols_module_t *ols_module_pointer;                         \
	MODULE_EXPORT void ols_module_set_pointer(ols_module_t *module); \
	void ols_module_set_pointer(ols_module_t *module)                \
	{                                                                \
		ols_module_pointer = module;                                 \
	}                                                                \
	ols_module_t *ols_current_module(void)                           \
	{                                                                \
		return ols_module_pointer;                                   \
	}                                                                \
	MODULE_EXPORT uint32_t ols_module_ver(void);                     \
	uint32_t ols_module_ver(void)                                    \
	{                                                                \
		return LIBOLS_API_VER;                                       \
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
#define OLS_MODULE_USE_DEFAULT_LOCALE(module_name, default_locale) \
	lookup_t *ols_module_lookup = NULL;                            \
	const char *ols_module_text(const char *val)                   \
	{                                                              \
		const char *out = val;                                     \
		text_lookup_getstr(ols_module_lookup, val, &out);          \
		return out;                                                \
	}                                                              \
	bool ols_module_get_string(const char *val, const char **out)  \
	{                                                              \
		return text_lookup_getstr(ols_module_lookup, val, out);    \
	}                                                              \
	void ols_module_set_locale(const char *locale)                 \
	{                                                              \
		if (ols_module_lookup)                                     \
			text_lookup_destroy(ols_module_lookup);                \
		ols_module_lookup = ols_module_load_locale(                \
			ols_current_module(), default_locale, locale);         \
	}                                                              \
	void ols_module_free_locale(void)                              \
	{                                                              \
		text_lookup_destroy(ols_module_lookup);                    \
		ols_module_lookup = NULL;                                  \
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
#define OLS_MODULE_AUTHOR(name)                        \
	MODULE_EXPORT const char *ols_module_author(void); \
	const char *ols_module_author(void)                \
	{                                                  \
		return name;                                   \
	}

/**
 * Opens a plugin module directly from a specific path.
 *
 * If the module already exists then the function will return successful, and
 * the module parameter will be given the pointer to the existing module.
 *
 * This does not initialize the module, it only loads the module image.  To
 * initialize the module, call ols_init_module.
 *
 * @param  module     The pointer to the created module.
 * @param  path       Specifies the path to the module library file.  If the
 *                    extension is not specified, it will use the extension
 *                    appropriate to the operating system.
 * @param  data_path  Specifies the path to the directory where the module's
 *                    data files are stored.
 * @returns           MODULE_SUCCESS if successful
 *                    MODULE_ERROR if a generic error occurred
 *                    MODULE_FILE_NOT_FOUND if the module was not found
 *                    MODULE_MISSING_EXPORTS if required exports are missing
 *                    MODULE_INCOMPATIBLE_VER if incompatible version
 */
EXPORT int ols_open_module(ols_module_t **module, const char *path,
						   const char *data_path);

/**
 * Initializes the module, which calls its ols_module_load export.  If the
 * module is already loaded, then this function does nothing and returns
 * successful.
 */
EXPORT bool ols_init_module(ols_module_t *module);

/** Returns a module based upon its name, or NULL if not found */
EXPORT ols_module_t *ols_get_module(const char *name);

/** Gets library of module */
EXPORT void *ols_get_module_lib(ols_module_t *module);

/** Returns locale text from a specific module */
EXPORT bool ols_module_get_locale_string(const ols_module_t *mod,
										 const char *lookup_string,
										 const char **translated_string);

EXPORT const char *ols_module_get_locale_text(const ols_module_t *mod,
											  const char *text);

/** Logs loaded modules */
EXPORT void ols_log_loaded_modules(void);

/** Returns the module file name */
EXPORT const char *ols_get_module_file_name(ols_module_t *module);

/** Returns the module full name */
EXPORT const char *ols_get_module_name(ols_module_t *module);

/** Returns the module author(s) */
EXPORT const char *ols_get_module_author(ols_module_t *module);

/** Returns the module description */
EXPORT const char *ols_get_module_description(ols_module_t *module);

/** Returns the module binary path */
EXPORT const char *ols_get_module_binary_path(ols_module_t *module);

/** Returns the module data path */
EXPORT const char *ols_get_module_data_path(ols_module_t *module);

#ifndef SWIG
/**
 * Adds a module search path to be used with ols_find_modules.  If the search
 * path strings contain %module%, that text will be replaced with the module
 * name when used.
 *
 * @param  bin   Specifies the module's binary directory search path.
 * @param  data  Specifies the module's data directory search path.
 */
EXPORT void ols_add_module_path(const char *bin, const char *data);

/**
 * Adds a module to the list of modules allowed to load in Safe Mode.
 * If the list is empty, all modules are allowed.
 *
 * @param  name  Specifies the module's name (filename sans extension).
 */
EXPORT void ols_add_safe_module(const char *name);

/** Automatically loads all modules from module paths (convenience function) */
EXPORT void ols_load_all_modules(void);

struct ols_module_failure_info
{
	char **failed_modules;
	size_t count;
};

EXPORT void ols_module_failure_info_free(struct ols_module_failure_info *mfi);
EXPORT void ols_load_all_modules2(struct ols_module_failure_info *mfi);

/** Notifies modules that all modules have been loaded.  This function should
 * be called after all modules have been loaded. */
EXPORT void ols_post_load_modules(void);

struct ols_module_info
{
	const char *bin_path;
	const char *data_path;
};

typedef void (*ols_find_module_callback_t)(void *param,
										   const struct ols_module_info *info);

/** Finds all modules within the search paths added by ols_add_module_path. */
EXPORT void ols_find_modules(ols_find_module_callback_t callback, void *param);

struct ols_module_info2
{
	const char *bin_path;
	const char *data_path;
	const char *name;
};

typedef void (*ols_find_module_callback2_t)(
	void *param, const struct ols_module_info2 *info);

/** Finds all modules within the search paths added by ols_add_module_path. */
EXPORT void ols_find_modules2(ols_find_module_callback2_t callback,
							  void *param);
#endif

typedef void (*ols_enum_module_callback_t)(void *param, ols_module_t *module);

/** Enumerates all loaded modules */
EXPORT void ols_enum_modules(ols_enum_module_callback_t callback, void *param);

/** Helper function for using default module locale */
EXPORT lookup_t *ols_module_load_locale(ols_module_t *module,
										const char *default_locale,
										const char *locale);

/**
 * Returns the location of a plugin module data file.
 *
 * @note   Modules should use ols_module_file function defined in ols-module.h
 *         as a more elegant means of getting their files without having to
 *         specify the module parameter.
 *
 * @param  module  The module associated with the file to locate
 * @param  file    The file to locate
 * @return         Path string, or NULL if not found.  Use bfree to free string.
 */
EXPORT char *ols_find_module_file(ols_module_t *module, const char *file);

/**
 * Returns the path of a plugin module config file (whether it exists or not)
 *
 * @note   Modules should use ols_module_config_path function defined in
 *         ols-module.h as a more elegant means of getting their files without
 *         having to specify the module parameter.
 *
 * @param  module  The module associated with the path
 * @param  file    The file to get a path to
 * @return         Path string, or NULL if not found.  Use bfree to free string.
 */
EXPORT char *ols_module_get_config_path(ols_module_t *module, const char *file);

/** Enumerates all source types (inputs, filters, transitions, etc).  */
EXPORT bool ols_enum_source_types(size_t idx, const char **id);

/** Enumerates all available output types. */
EXPORT bool ols_enum_output_types(size_t idx, const char **id);

/** Optional: Returns the full name of the module */
MODULE_EXPORT const char *ols_module_name(void);

/** Optional: Returns a description of the module */
MODULE_EXPORT const char *ols_module_description(void);
