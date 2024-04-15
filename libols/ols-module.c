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

#include "util/platform.h"
#include "util/dstr.h"

#include "ols-defs.h"
#include "ols-internal.h"
#include "ols-module.h"

extern const char *get_module_extension(void);

static inline int req_func_not_found(const char *name, const char *path)
{
	blog(LOG_DEBUG,
	     "Required module function '%s' in module '%s' not "
	     "found, loading of module failed",
	     name, path);
	return MODULE_MISSING_EXPORTS;
}

static int load_module_exports(struct ols_module *mod, const char *path)
{
	mod->load = os_dlsym(mod->module, "ols_module_load");
	if (!mod->load)
		return req_func_not_found("ols_module_load", path);

	mod->set_pointer = os_dlsym(mod->module, "ols_module_set_pointer");
	if (!mod->set_pointer)
		return req_func_not_found("ols_module_set_pointer", path);

	mod->ver = os_dlsym(mod->module, "ols_module_ver");
	if (!mod->ver)
		return req_func_not_found("ols_module_ver", path);

	/* optional exports */
	mod->unload = os_dlsym(mod->module, "ols_module_unload");
	mod->post_load = os_dlsym(mod->module, "ols_module_post_load");
	mod->set_locale = os_dlsym(mod->module, "ols_module_set_locale");
	mod->free_locale = os_dlsym(mod->module, "ols_module_free_locale");
	mod->name = os_dlsym(mod->module, "ols_module_name");
	mod->description = os_dlsym(mod->module, "ols_module_description");
	mod->author = os_dlsym(mod->module, "ols_module_author");
	mod->get_string = os_dlsym(mod->module, "ols_module_get_string");
	return MODULE_SUCCESS;
}

bool ols_module_get_locale_string(const ols_module_t *mod,
				  const char *lookup_string,
				  const char **translated_string)
{
	if (mod->get_string) {
		return mod->get_string(lookup_string, translated_string);
	}

	return false;
}

const char *ols_module_get_locale_text(const ols_module_t *mod,
				       const char *text)
{
	const char *str = text;
	ols_module_get_locale_string(mod, text, &str);
	return str;
}

static inline char *get_module_name(const char *file)
{
	static size_t ext_len = 0;
	struct dstr name = {0};

	if (ext_len == 0) {
		const char *ext = get_module_extension();
		ext_len = strlen(ext);
	}

	dstr_copy(&name, file);
	dstr_resize(&name, name.len - ext_len);
	return name.array;
}

#ifdef _WIN32
extern void reset_win32_symbol_paths(void);
#endif

int ols_open_module(ols_module_t **module, const char *path,
		    const char *data_path)
{
	struct ols_module mod = {0};
	int errorcode;

	if (!module || !path || !ols)
		return MODULE_ERROR;

#ifdef __APPLE__
	/* HACK: Do not load olsolete ols-browser build on macOS; the
	 * ols-browser plugin used to live in the Application Support
	 * directory. */
	if (astrstri(path, "Library/Application Support/ols-studio") != NULL &&
	    astrstri(path, "ols-browser") != NULL) {
		blog(LOG_WARNING, "Ignoring old ols-browser.so version");
		return MODULE_HARDCODED_SKIP;
	}
#endif

	blog(LOG_DEBUG, "---------------------------------");

	mod.module = os_dlopen(path);
	if (!mod.module) {
		blog(LOG_WARNING, "Module '%s' not loaded", path);
		return MODULE_FILE_NOT_FOUND;
	}

	errorcode = load_module_exports(&mod, path);
	if (errorcode != MODULE_SUCCESS)
		return errorcode;

	mod.bin_path = bstrdup(path);
	mod.file = strrchr(mod.bin_path, '/');
	mod.file = (!mod.file) ? mod.bin_path : (mod.file + 1);
	mod.mod_name = get_module_name(mod.file);
	mod.data_path = bstrdup(data_path);
	mod.next = ols->first_module;

	if (mod.file) {
		blog(LOG_DEBUG, "Loading module: %s", mod.file);
	}

	*module = bmemdup(&mod, sizeof(mod));
	ols->first_module = (*module);
	mod.set_pointer(*module);

	if (mod.set_locale)
		mod.set_locale(ols->locale);

	return MODULE_SUCCESS;
}

bool ols_init_module(ols_module_t *module)
{
	if (!module || !ols)
		return false;
	if (module->loaded)
		return true;

	const char *profile_name =
		profile_store_name(ols_get_profiler_name_store(),
				   "ols_init_module(%s)", module->file);
	profile_start(profile_name);

	module->loaded = module->load();
	if (!module->loaded)
		blog(LOG_WARNING, "Failed to initialize module '%s'",
		     module->file);

	profile_end(profile_name);
	return module->loaded;
}

void ols_log_loaded_modules(void)
{
	blog(LOG_INFO, "  Loaded Modules:");

	for (ols_module_t *mod = ols->first_module; !!mod; mod = mod->next)
		blog(LOG_INFO, "    %s", mod->file);
}

const char *ols_get_module_file_name(ols_module_t *module)
{
	return module ? module->file : NULL;
}

const char *ols_get_module_name(ols_module_t *module)
{
	return (module && module->name) ? module->name() : NULL;
}

const char *ols_get_module_author(ols_module_t *module)
{
	return (module && module->author) ? module->author() : NULL;
}

const char *ols_get_module_description(ols_module_t *module)
{
	return (module && module->description) ? module->description() : NULL;
}

const char *ols_get_module_binary_path(ols_module_t *module)
{
	return module ? module->bin_path : NULL;
}

const char *ols_get_module_data_path(ols_module_t *module)
{
	return module ? module->data_path : NULL;
}

ols_module_t *ols_get_module(const char *name)
{
	ols_module_t *module = ols->first_module;
	while (module) {
		if (strcmp(module->mod_name, name) == 0) {
			return module;
		}

		module = module->next;
	}

	return NULL;
}

void *ols_get_module_lib(ols_module_t *module)
{
	return module ? module->module : NULL;
}

char *ols_find_module_file(ols_module_t *module, const char *file)
{
	struct dstr output = {0};

	if (!file)
		file = "";

	if (!module)
		return NULL;

	dstr_copy(&output, module->data_path);
	if (!dstr_is_empty(&output) && dstr_end(&output) != '/' && *file)
		dstr_cat_ch(&output, '/');
	dstr_cat(&output, file);

	if (!os_file_exists(output.array))
		dstr_free(&output);
	return output.array;
}

char *ols_module_get_config_path(ols_module_t *module, const char *file)
{
	struct dstr output = {0};

	dstr_copy(&output, ols->module_config_path);
	if (!dstr_is_empty(&output) && dstr_end(&output) != '/')
		dstr_cat_ch(&output, '/');
	dstr_cat(&output, module->mod_name);
	dstr_cat_ch(&output, '/');
	dstr_cat(&output, file);

	return output.array;
}

void ols_add_module_path(const char *bin, const char *data)
{
	struct ols_module_path omp;

	if (!ols || !bin || !data)
		return;

	omp.bin = bstrdup(bin);
	omp.data = bstrdup(data);
	da_push_back(ols->module_paths, &omp);
}

void ols_add_safe_module(const char *name)
{
	if (!ols || !name)
		return;

	char *item = bstrdup(name);
	da_push_back(ols->safe_modules, &item);
}

extern void get_plugin_info(const char *path, bool *is_ols_plugin,
			    bool *can_load);

struct fail_info {
	struct dstr fail_modules;
	size_t fail_count;
};

static bool is_safe_module(const char *name)
{
	if (!ols->safe_modules.num)
		return true;

	for (size_t i = 0; i < ols->safe_modules.num; i++) {
		if (strcmp(name, ols->safe_modules.array[i]) == 0)
			return true;
	}

	return false;
}

static void load_all_callback(void *param, const struct ols_module_info2 *info)
{
	struct fail_info *fail_info = param;
	ols_module_t *module;

	bool is_ols_plugin;
	bool can_load_ols_plugin;

	get_plugin_info(info->bin_path, &is_ols_plugin, &can_load_ols_plugin);

	if (!is_ols_plugin) {
		blog(LOG_WARNING, "Skipping module '%s', not an OLS plugin",
		     info->bin_path);
		return;
	}

	if (!is_safe_module(info->name)) {
		blog(LOG_WARNING, "Skipping module '%s', not on safe list",
		     info->name);
		return;
	}

	if (!can_load_ols_plugin) {
		blog(LOG_WARNING,
		     "Skipping module '%s' due to possible "
		     "import conflicts",
		     info->bin_path);
		goto load_failure;
	}

	int code = ols_open_module(&module, info->bin_path, info->data_path);
	switch (code) {
	case MODULE_MISSING_EXPORTS:
		blog(LOG_DEBUG,
		     "Failed to load module file '%s', not an OLS plugin",
		     info->bin_path);
		return;
	case MODULE_FILE_NOT_FOUND:
		blog(LOG_DEBUG,
		     "Failed to load module file '%s', file not found",
		     info->bin_path);
		return;
	case MODULE_ERROR:
		blog(LOG_DEBUG, "Failed to load module file '%s'",
		     info->bin_path);
		goto load_failure;
	case MODULE_INCOMPATIBLE_VER:
		blog(LOG_DEBUG,
		     "Failed to load module file '%s', incompatible version",
		     info->bin_path);
		goto load_failure;
	case MODULE_HARDCODED_SKIP:
		return;
	}

	if (!ols_init_module(module))
		free_module(module);

	UNUSED_PARAMETER(param);
	return;

load_failure:
	if (fail_info) {
		dstr_cat(&fail_info->fail_modules, info->name);
		dstr_cat(&fail_info->fail_modules, ";");
		fail_info->fail_count++;
	}
}

static const char *ols_load_all_modules_name = "ols_load_all_modules";
#ifdef _WIN32
static const char *reset_win32_symbol_paths_name = "reset_win32_symbol_paths";
#endif

void ols_load_all_modules(void)
{
	profile_start(ols_load_all_modules_name);
	ols_find_modules2(load_all_callback, NULL);
#ifdef _WIN32
	profile_start(reset_win32_symbol_paths_name);
	reset_win32_symbol_paths();
	profile_end(reset_win32_symbol_paths_name);
#endif
	profile_end(ols_load_all_modules_name);
}

static const char *ols_load_all_modules2_name = "ols_load_all_modules2";

void ols_load_all_modules2(struct ols_module_failure_info *mfi)
{
	struct fail_info fail_info = {0};
	memset(mfi, 0, sizeof(*mfi));

	profile_start(ols_load_all_modules2_name);
	ols_find_modules2(load_all_callback, &fail_info);
#ifdef _WIN32
	profile_start(reset_win32_symbol_paths_name);
	reset_win32_symbol_paths();
	profile_end(reset_win32_symbol_paths_name);
#endif
	profile_end(ols_load_all_modules2_name);

	mfi->count = fail_info.fail_count;
	mfi->failed_modules =
		strlist_split(fail_info.fail_modules.array, ';', false);
	dstr_free(&fail_info.fail_modules);
}

void ols_module_failure_info_free(struct ols_module_failure_info *mfi)
{
	if (mfi->failed_modules) {
		bfree(mfi->failed_modules);
		mfi->failed_modules = NULL;
	}
}

void ols_post_load_modules(void)
{
	for (ols_module_t *mod = ols->first_module; !!mod; mod = mod->next)
		if (mod->post_load)
			mod->post_load();
}

static inline void make_data_dir(struct dstr *parsed_data_dir,
				 const char *data_dir, const char *name)
{
	dstr_copy(parsed_data_dir, data_dir);
	dstr_replace(parsed_data_dir, "%module%", name);
	if (dstr_end(parsed_data_dir) == '/')
		dstr_resize(parsed_data_dir, parsed_data_dir->len - 1);
}

static char *make_data_directory(const char *module_name, const char *data_dir)
{
	struct dstr parsed_data_dir = {0};
	bool found = false;

	make_data_dir(&parsed_data_dir, data_dir, module_name);

	found = os_file_exists(parsed_data_dir.array);

	if (!found && astrcmpi_n(module_name, "lib", 3) == 0)
		make_data_dir(&parsed_data_dir, data_dir, module_name + 3);

	return parsed_data_dir.array;
}

static bool parse_binary_from_directory(struct dstr *parsed_bin_path,
					const char *bin_path, const char *file)
{
	struct dstr directory = {0};
	bool found = true;

	dstr_copy(&directory, bin_path);
	dstr_replace(&directory, "%module%", file);
	if (dstr_end(&directory) != '/')
		dstr_cat_ch(&directory, '/');

	dstr_copy_dstr(parsed_bin_path, &directory);
	dstr_cat(parsed_bin_path, file);
#ifdef __APPLE__
	if (!os_file_exists(parsed_bin_path->array)) {
		dstr_cat(parsed_bin_path, ".so");
	}
#else
	dstr_cat(parsed_bin_path, get_module_extension());
#endif

	if (!os_file_exists(parsed_bin_path->array)) {
		/* Legacy fallback: Check for plugin with .so suffix*/
		dstr_cat(parsed_bin_path, ".so");
		/* if the file doesn't exist, check with 'lib' prefix */
		dstr_copy_dstr(parsed_bin_path, &directory);
		dstr_cat(parsed_bin_path, "lib");
		dstr_cat(parsed_bin_path, file);
		dstr_cat(parsed_bin_path, get_module_extension());

		/* if neither exist, don't include this as a library */
		if (!os_file_exists(parsed_bin_path->array)) {
			dstr_free(parsed_bin_path);
			found = false;
		}
	}

	dstr_free(&directory);
	return found;
}

static void process_found_module(struct ols_module_path *omp, const char *path,
				 bool directory,
				 ols_find_module_callback2_t callback,
				 void *param)
{
	struct ols_module_info2 info;
	struct dstr name = {0};
	struct dstr parsed_bin_path = {0};
	const char *file;
	char *parsed_data_dir;
	bool bin_found = true;

	file = strrchr(path, '/');
	file = file ? (file + 1) : path;

	if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0)
		return;

	dstr_copy(&name, file);
	char *ext = strrchr(name.array, '.');
	if (ext)
		dstr_resize(&name, ext - name.array);

	if (!directory) {
		dstr_copy(&parsed_bin_path, path);
	} else {
		bin_found = parse_binary_from_directory(&parsed_bin_path,
							omp->bin, name.array);
	}

	parsed_data_dir = make_data_directory(name.array, omp->data);

	if (parsed_data_dir && bin_found) {
		info.bin_path = parsed_bin_path.array;
		info.data_path = parsed_data_dir;
		info.name = name.array;
		callback(param, &info);
	}

	bfree(parsed_data_dir);
	dstr_free(&name);
	dstr_free(&parsed_bin_path);
}

static void find_modules_in_path(struct ols_module_path *omp,
				 ols_find_module_callback2_t callback,
				 void *param)
{
	struct dstr search_path = {0};
	char *module_start;
	bool search_directories = false;
	os_glob_t *gi;

	dstr_copy(&search_path, omp->bin);

	module_start = strstr(search_path.array, "%module%");
	if (module_start) {
		dstr_resize(&search_path, module_start - search_path.array);
		search_directories = true;
	}

	if (!dstr_is_empty(&search_path) && dstr_end(&search_path) != '/')
		dstr_cat_ch(&search_path, '/');

	dstr_cat_ch(&search_path, '*');
	if (!search_directories)
		dstr_cat(&search_path, get_module_extension());

	if (os_glob(search_path.array, 0, &gi) == 0) {
		for (size_t i = 0; i < gi->gl_pathc; i++) {
			if (search_directories == gi->gl_pathv[i].directory)
				process_found_module(omp, gi->gl_pathv[i].path,
						     search_directories,
						     callback, param);
		}

		os_globfree(gi);
	}

	dstr_free(&search_path);
}

void ols_find_modules2(ols_find_module_callback2_t callback, void *param)
{
	if (!ols)
		return;

	for (size_t i = 0; i < ols->module_paths.num; i++) {
		struct ols_module_path *omp = ols->module_paths.array + i;
		find_modules_in_path(omp, callback, param);
	}
}

void ols_find_modules(ols_find_module_callback_t callback, void *param)
{
	/* the structure is ABI compatible so we can just cast the callback */
	ols_find_modules2((ols_find_module_callback2_t)callback, param);
}

void ols_enum_modules(ols_enum_module_callback_t callback, void *param)
{
	struct ols_module *module;
	if (!ols)
		return;

	module = ols->first_module;
	while (module) {
		callback(param, module);
		module = module->next;
	}
}

void free_module(struct ols_module *mod)
{
	if (!mod)
		return;

	if (mod->module) {
		if (mod->free_locale)
			mod->free_locale();

		if (mod->loaded && mod->unload)
			mod->unload();

		/* there is no real reason to close the dynamic libraries,
		 * and sometimes this can cause issues. */
		/* os_dlclose(mod->module); */
	}

	for (ols_module_t *m = ols->first_module; !!m; m = m->next) {
		if (m->next == mod) {
			m->next = mod->next;
			break;
		}
	}

	if (ols->first_module == mod)
		ols->first_module = mod->next;

	bfree(mod->mod_name);
	bfree(mod->bin_path);
	bfree(mod->data_path);
	bfree(mod);
}

lookup_t *ols_module_load_locale(ols_module_t *module,
				 const char *default_locale, const char *locale)
{
	struct dstr str = {0};
	lookup_t *lookup = NULL;

	if (!module || !default_locale || !locale) {
		blog(LOG_WARNING, "ols_module_load_locale: Invalid parameters");
		return NULL;
	}

	dstr_copy(&str, "locale/");
	dstr_cat(&str, default_locale);
	dstr_cat(&str, ".ini");

	char *file = ols_find_module_file(module, str.array);
	if (file)
		lookup = text_lookup_create(file);

	bfree(file);

	if (!lookup) {
		blog(LOG_WARNING, "Failed to load '%s' text for module: '%s'",
		     default_locale, module->file);
		goto cleanup;
	}

	if (astrcmpi(locale, default_locale) == 0)
		goto cleanup;

	dstr_copy(&str, "/locale/");
	dstr_cat(&str, locale);
	dstr_cat(&str, ".ini");

	file = ols_find_module_file(module, str.array);

	if (!text_lookup_add(lookup, file))
		blog(LOG_WARNING, "Failed to load '%s' text for module: '%s'",
		     locale, module->file);

	bfree(file);
cleanup:
	dstr_free(&str);
	return lookup;
}

#define REGISTER_OLS_DEF(size_var, structure, dest, info)               \
	do {                                                            \
		struct structure data = {0};                            \
		if (!size_var) {                                        \
			blog(LOG_ERROR, "Tried to register " #structure \
					" outside of ols_module_load"); \
			return;                                         \
		}                                                       \
                                                                        \
		if (size_var > sizeof(data)) {                          \
			blog(LOG_ERROR,                                 \
			     "Tried to register " #structure            \
			     " with size %llu which is more "           \
			     "than libols currently supports "          \
			     "(%llu)",                                  \
			     (long long unsigned)size_var,              \
			     (long long unsigned)sizeof(data));         \
			goto error;                                     \
		}                                                       \
                                                                        \
		memcpy(&data, info, size_var);                          \
		da_push_back(dest, &data);                              \
	} while (false)

#define HAS_VAL(type, info, val) \
	((offsetof(type, val) + sizeof(info->val) <= size) && info->val)

#define CHECK_REQUIRED_VAL(type, info, val, func)                  \
	do {                                                       \
		if (!HAS_VAL(type, info, val)) {                   \
			blog(LOG_ERROR,                            \
			     "Required value '" #val "' for "      \
			     "'%s' not found.  " #func " failed.", \
			     info->id);                            \
			goto error;                                \
		}                                                  \
	} while (false)

#define CHECK_REQUIRED_VAL_EITHER(type, info, val1, val2, func)     \
	do {                                                        \
		if (!HAS_VAL(type, info, val1) &&                   \
		    !HAS_VAL(type, info, val2)) {                   \
			blog(LOG_ERROR,                             \
			     "Neither '" #val1 "' nor '" #val2 "' " \
			     "for '%s' found.  " #func " failed.",  \
			     info->id);                             \
			goto error;                                 \
		}                                                   \
	} while (false)

#define HANDLE_ERROR(size_var, structure, info)                            \
	do {                                                               \
		struct structure data = {0};                               \
		if (!size_var)                                             \
			return;                                            \
                                                                           \
		memcpy(&data, info,                                        \
		       sizeof(data) < size_var ? sizeof(data) : size_var); \
                                                                           \
		if (data.type_data && data.free_type_data)                 \
			data.free_type_data(data.type_data);               \
	} while (false)

#define source_warn(format, ...) \
	blog(LOG_WARNING, "ols_register_source: " format, ##__VA_ARGS__)
#define output_warn(format, ...) \
	blog(LOG_WARNING, "ols_register_output: " format, ##__VA_ARGS__)
#define encoder_warn(format, ...) \
	blog(LOG_WARNING, "ols_register_encoder: " format, ##__VA_ARGS__)
#define service_warn(format, ...) \
	blog(LOG_WARNING, "ols_register_service: " format, ##__VA_ARGS__)

void ols_register_source_s(const struct ols_source_info *info, size_t size)
{
	struct ols_source_info data = {0};
	ols_source_info_array_t *array = NULL;

	if (info->type == OLS_SOURCE_TYPE_INPUT) {
		array = &ols->input_types;
	} else if (info->type == OLS_SOURCE_TYPE_FILTER) {
		array = &ols->filter_types;
	} else {
		source_warn("Tried to register unknown source type: %u",
			    info->type);
		goto error;
	}

	if (get_source_info2(info->id, info->version)) {
		source_warn("Source '%s' already exists!  "
			    "Duplicate library?",
			    info->id);
		goto error;
	}

	if (size > sizeof(data)) {
		source_warn("Tried to register ols_source_info with size "
			    "%llu which is more than libols currently "
			    "supports (%llu)",
			    (long long unsigned)size,
			    (long long unsigned)sizeof(data));
		goto error;
	}

	memcpy(&data, info, size);

	/* version-related stuff */
	data.unversioned_id = data.id;
	if (data.version) {
		struct dstr versioned_id = {0};
		dstr_printf(&versioned_id, "%s_v%d", data.id,
			    (int)data.version);
		data.id = versioned_id.array;
	} else {
		data.id = bstrdup(data.id);
	}

	if (array)
		da_push_back(*array, &data);
	da_push_back(ols->source_types, &data);
	return;

error:
	HANDLE_ERROR(size, ols_source_info, info);
}

void ols_register_output_s(const struct ols_output_info *info, size_t size)
{
	if (find_output(info->id)) {
		output_warn("Output id '%s' already exists!  "
			    "Duplicate library?",
			    info->id);
		goto error;
	}

#define CHECK_REQUIRED_VAL_(info, val, func) \
	CHECK_REQUIRED_VAL(struct ols_output_info, info, val, func)
	CHECK_REQUIRED_VAL_(info, get_name, ols_register_output);
	CHECK_REQUIRED_VAL_(info, create, ols_register_output);
	CHECK_REQUIRED_VAL_(info, destroy, ols_register_output);
	CHECK_REQUIRED_VAL_(info, start, ols_register_output);
	CHECK_REQUIRED_VAL_(info, stop, ols_register_output);


#undef CHECK_REQUIRED_VAL_

	REGISTER_OLS_DEF(size, ols_output_info, ols->output_types, info);


	return;

error:
	HANDLE_ERROR(size, ols_output_info, info);
}



void ols_register_service_s(const struct ols_service_info *info, size_t size)
{
	if (find_service(info->id)) {
		service_warn("Service id '%s' already exists!  "
			     "Duplicate library?",
			     info->id);
		goto error;
	}

#define CHECK_REQUIRED_VAL_(info, val, func) \
	CHECK_REQUIRED_VAL(struct ols_service_info, info, val, func)
	CHECK_REQUIRED_VAL_(info, get_name, ols_register_service);
	CHECK_REQUIRED_VAL_(info, create, ols_register_service);
	CHECK_REQUIRED_VAL_(info, destroy, ols_register_service);
	CHECK_REQUIRED_VAL_(info, get_protocol, ols_register_service);
#undef CHECK_REQUIRED_VAL_

	REGISTER_OLS_DEF(size, ols_service_info, ols->service_types, info);
	return;

error:
	HANDLE_ERROR(size, ols_service_info, info);
}
