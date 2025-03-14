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

#include "callback/proc.h"
#include "callback/signal.h"
#include "util/bmem.h"
#include "util/c99defs.h"
#include "util/profiler.h"
#include "util/text-lookup.h"

#include "ols-buffer-list.h"
#include "ols-buffer.h"
#include "ols-config.h"
#include "ols-data.h"
#include "ols-defs.h"
#include "ols-event.h"
#include "ols-mini-object.h"
#include "ols-properties.h"

/* opaque types */
struct ols_context_data;
struct ols_source;
struct ols_process;
struct ols_output;

struct ols_module;

typedef struct ols_context_data ols_object_t;

typedef struct ols_source ols_source_t;
typedef struct ols_process ols_process_t;
typedef struct ols_output ols_output_t;
typedef struct ols_module ols_module_t;

typedef struct ols_weak_object ols_weak_object_t;
typedef struct ols_weak_source ols_weak_source_t;
typedef struct ols_weak_process ols_weak_process_t;
typedef struct ols_weak_output ols_weak_output_t;

#include "ols-output.h"
#include "ols-pad.h"
#include "ols-process.h"
#include "ols-source.h"

/**
 * @file
 * @brief Main libols header used by applications.
 *
 * @mainpage
 *
 * @section intro_sec Introduction
 *
 * This document describes the api for libols to be used by applications as well
 * as @ref modules_page implementing some kind of functionality.
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif

  /** Access to the argc/argv used to start OLS. What you see is what you get. */
  struct ols_cmdline_args
  {
    int argc;
    char **argv;
  };

#define OLS_OBJECT_CAST(obj) ((ols_object_t *)(obj))

/**
 * OLS_OBJECT_GET_LOCK:
 * @obj: a #ols_object
 *
 * Acquire a reference to the mutex of this object.
 */
#define OLS_OBJECT_GET_LOCK(obj) (&OLS_OBJECT_CAST(obj)->mutex)
/**
 * OLS_OBJECT_LOCK:
 * @obj: a #ols_object_t to lock
 *
 * This macro will obtain a lock on the object, making serialization possible.
 * It blocks until the lock can be obtained.
 */
#define OLS_OBJECT_LOCK(obj) pthread_mutex_lock(OLS_OBJECT_GET_LOCK(obj))

/**
 * OLS_OBJECT_TRYLOCK:
 * @obj: a #ols_object_t.
 *
 * This macro will try to obtain a lock on the object, but will return with
 * %FALSE if it can't get it immediately.
 */
#define OLS_OBJECT_TRYLOCK(obj) pthread_mutex_trylock(OLS_OBJECT_GET_LOCK(obj))
/**
 * OLS_OBJECT_UNLOCK:
 * @obj: a #ols_object_t to unlock.
 *
 * This macro releases a lock on the object.
 */
#define OLS_OBJECT_UNLOCK(obj) pthread_mutex_unlock(OLS_OBJECT_GET_LOCK(obj))

  /* ------------------------------------------------------------------------- */
  /* OLS context */

  /**
   * Find a core libols data file
   * @param path name of the base file
   * @return A string containing the full path to the file.
   *          Use bfree after use.
   */
  EXPORT char *ols_find_data_file(const char *file);

  /**
   * Add a path to search libols data files in.
   * @param path Full path to directory to look in.
   *             The string is copied.
   */
  EXPORT void ols_add_data_path(const char *path);

  /**
   * Remove a path from libols core data paths.
   * @param path The path to compare to currently set paths.
   *             It does not need to be the same pointer, but
   *             the path string must match an entry fully.
   * @return Whether or not the path was successfully removed.
   *         If false, the path could not be found.
   */
  EXPORT bool ols_remove_data_path(const char *path);

  /**
   * Initializes OLS
   *
   * @param  locale              The locale to use for modules
   * @param  module_config_path  Path to module config storage directory
   *                             (or NULL if none)
   * @param  store               The profiler name store for OLS to use or NULL
   */
  EXPORT bool ols_startup(const char *locale, const char *module_config_path,
                          profiler_name_store_t *store);

  /** Releases all data associated with OLS and terminates the OLS context */
  EXPORT void ols_shutdown(void);

  /** @return true if the main OLS context has been initialized */
  EXPORT bool ols_initialized(void);

  /** @return The current core version */
  EXPORT uint32_t ols_get_version(void);

  /** @return The current core version string */
  EXPORT const char *ols_get_version_string(void);

  /**
   * Sets things up for calls to ols_get_cmdline_args. Called only once at startup
   * and safely copies argv/argc from main(). Subsequent calls do nothing.
   *
   * @param  argc  The count of command line arguments, from main()
   * @param  argv  An array of command line arguments, copied from main() and ends
   *               with NULL.
   */
  EXPORT void ols_set_cmdline_args(int argc, const char *const *argv);

  /**
   * Get the argc/argv used to start OLS
   *
   * @return  The command line arguments used for main(). Don't modify this or
   *          you'll mess things up for other callers.
   */
  EXPORT struct ols_cmdline_args ols_get_cmdline_args(void);

  /**
   * Sets a new locale to use for modules.  This will call ols_module_set_locale
   * for each module with the new locale.
   *
   * @param  locale  The locale to use for modules
   */
  EXPORT void ols_set_locale(const char *locale);

  /** @return the current locale */
  EXPORT const char *ols_get_locale(void);

  /** Initialize the Windows-specific crash handler */

#ifdef _WIN32
  EXPORT void ols_init_win32_crash_handler(void);
#endif

  /**
   * Returns the profiler name store (see util/profiler.h) used by OLS, which is
   * either a name store passed to ols_startup, an internal name store, or NULL
   * in case ols_initialized() returns false.
   */
  EXPORT profiler_name_store_t *ols_get_profiler_name_store(void);

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

  /**
   * Enumerates all input sources
   *
   *   Callback function returns true to continue enumeration, or false to end
   * enumeration.
   *
   *   Use ols_source_get_ref or ols_source_get_weak_source if you want to retain
   * a reference after ols_enum_sources finishes
   */
  EXPORT void ols_enum_sources(bool (*enum_proc)(void *, ols_source_t *),
                               void *param);

  /** Enumerates all sources (regardless of type) */
  EXPORT void ols_enum_all_sources(bool (*enum_proc)(void *, ols_source_t *),
                                   void *param);

  /** Enumerates outputs */
  EXPORT void ols_enum_outputs(bool (*enum_proc)(void *, ols_output_t *),
                               void *param);

  /**
   * Gets a source by its UUID.
   *
   *   Increments the source reference counter, use ols_source_release to
   * release it when complete.
   */
  EXPORT ols_source_t *ols_get_source_by_uuid(const char *uuid);

  /** Returns the primary ols signal handler */
  EXPORT signal_handler_t *ols_get_signal_handler(void);

  /** Returns the primary ols procedure handler */
  EXPORT proc_handler_t *ols_get_proc_handler(void);

  /** Saves a source to settings data */
  EXPORT ols_data_t *ols_save_source(ols_source_t *source);

  /** Loads a source from settings data */
  EXPORT ols_source_t *ols_load_source(ols_data_t *data);

  /** Loads a private source from settings data */
  EXPORT ols_source_t *ols_load_private_source(ols_data_t *data);

  /** Send a save signal to sources */
  EXPORT void ols_source_save(ols_source_t *source);

  /** Send a load signal to sources (soft deprecated; does not load filters) */
  EXPORT void ols_source_load(ols_source_t *source);

  typedef void (*ols_load_source_cb)(void *private_data, ols_source_t *source);

  /** Loads sources from a data array */
  EXPORT void ols_load_sources(ols_data_array_t *array, ols_load_source_cb cb,
                               void *private_data);

  /** Saves sources to a data array */
  EXPORT ols_data_array_t *ols_save_sources(void);

  typedef bool (*ols_save_source_filter_cb)(void *data, ols_source_t *source);
  EXPORT ols_data_array_t *ols_save_sources_filtered(ols_save_source_filter_cb cb,
                                                     void *data);

  /** Reset source UUIDs. NOTE: this function is only to be used by the UI and
   *  will be removed in a future version! */
  EXPORT void ols_reset_source_uuids(void);

  enum ols_obj_type
  {
    OLS_OBJ_TYPE_INVALID,
    OLS_OBJ_TYPE_SOURCE,
    OLS_OBJ_TYPE_PROCESS,
    OLS_OBJ_TYPE_OUTPUT,
  };

  EXPORT enum ols_obj_type ols_obj_get_type(void *obj);
  EXPORT const char *ols_obj_get_id(void *obj);
  EXPORT bool ols_obj_invalid(void *obj);
  EXPORT void *ols_obj_get_data(void *obj);
  EXPORT bool ols_obj_is_private(void *obj);

  EXPORT void ols_apply_private_data(ols_data_t *settings);
  EXPORT void ols_set_private_data(ols_data_t *settings);
  EXPORT ols_data_t *ols_get_private_data(void);

  typedef void (*ols_task_t)(void *param);

  enum ols_task_type
  {
    OLS_TASK_UI,
    OLS_TASK_DESTROY,
  };

  EXPORT void ols_queue_task(enum ols_task_type type, ols_task_t task,
                             void *param, bool wait);
  EXPORT bool ols_in_task_thread(enum ols_task_type type);

  EXPORT bool ols_wait_for_destroy_queue(void);

  typedef void (*ols_task_handler_t)(ols_task_t task, void *param, bool wait);
  EXPORT void ols_set_ui_task_handler(ols_task_handler_t handler);

  EXPORT ols_object_t *ols_object_get_ref(ols_object_t *object);
  EXPORT void ols_object_release(ols_object_t *object);

  EXPORT void ols_weak_object_addref(ols_weak_object_t *weak);
  EXPORT void ols_weak_object_release(ols_weak_object_t *weak);
  EXPORT ols_weak_object_t *ols_object_get_weak_object(ols_object_t *object);
  EXPORT ols_object_t *ols_weak_object_get_object(ols_weak_object_t *weak);
  EXPORT bool ols_weak_object_expired(ols_weak_object_t *weak);
  EXPORT bool ols_weak_object_references_object(ols_weak_object_t *weak,
                                                ols_object_t *object);

  /* ------------------------------------------------------------------------- */
  /* Sources */

  /** Returns the translated display name of a source */
  EXPORT const char *ols_source_get_display_name(const char *id);

  /**
   * Creates a source of the specified type with the specified settings.
   *
   *   The "source" context is used for anything related to presenting
   * or modifying video/audio.  Use ols_source_release to release it.
   */
  EXPORT ols_source_t *ols_source_create(const char *id, const char *name,
                                         ols_data_t *settings);

  EXPORT ols_source_t *ols_source_create_private(const char *id, const char *name,
                                                 ols_data_t *settings);

  /* if source has OLS_SOURCE_DO_NOT_DUPLICATE output flag set, only returns a
   * reference */
  EXPORT ols_source_t *ols_source_duplicate(ols_source_t *source,
                                            const char *desired_name,
                                            bool create_private);
  /**
   * Adds/releases a reference to a source.  When the last reference is
   * released, the source is destroyed.
   */
  OLS_EXTERNAL_DEPRECATED EXPORT void ols_source_addref(ols_source_t *source);
  EXPORT void ols_source_release(ols_source_t *source);

  EXPORT void ols_weak_source_addref(ols_weak_source_t *weak);
  EXPORT void ols_weak_source_release(ols_weak_source_t *weak);

  EXPORT ols_source_t *ols_source_get_ref(ols_source_t *source);
  EXPORT ols_weak_source_t *ols_source_get_weak_source(ols_source_t *source);
  EXPORT ols_source_t *ols_weak_source_get_source(ols_weak_source_t *weak);
  EXPORT bool ols_weak_source_expired(ols_weak_source_t *weak);

  EXPORT bool ols_weak_source_references_source(ols_weak_source_t *weak,
                                                ols_source_t *source);

  /** Notifies all references that the source should be released */
  EXPORT void ols_source_remove(ols_source_t *source);

  /** Returns true if the source should be released */
  EXPORT bool ols_source_removed(const ols_source_t *source);

  /** Returns capability flags of a source */
  EXPORT uint32_t ols_source_get_output_flags(const ols_source_t *source);

  /** Returns capability flags of a source type */
  EXPORT uint32_t ols_get_source_output_flags(const char *id);

  /** Gets the default settings for a source type */
  EXPORT ols_data_t *ols_get_source_defaults(const char *id);

  /** Returns the property list, if any.  Free with ols_properties_destroy */
  EXPORT ols_properties_t *ols_get_source_properties(const char *id);

  /** Returns whether the source has custom properties or not */
  EXPORT bool ols_is_source_configurable(const char *id);

  EXPORT bool ols_source_configurable(const ols_source_t *source);

  /**
   * Returns the properties list for a specific existing source.  Free with
   * ols_properties_destroy
   */
  EXPORT ols_properties_t *ols_source_properties(const ols_source_t *source);

  /** Updates settings for this source */
  EXPORT void ols_source_update(ols_source_t *source, ols_data_t *settings);
  EXPORT void ols_source_reset_settings(ols_source_t *source,
                                        ols_data_t *settings);

  /** Gets the settings string for a source */
  EXPORT ols_data_t *ols_source_get_settings(const ols_source_t *source);

  /** Gets the name of a source */
  EXPORT const char *ols_source_get_name(const ols_source_t *source);

  /** Sets the name of a source */
  EXPORT void ols_source_set_name(ols_source_t *source, const char *name);

  /** Gets the UUID of a source */
  EXPORT const char *ols_source_get_uuid(const ols_source_t *source);

  /** Gets the source type */
  EXPORT enum ols_source_type ols_source_get_type(const ols_source_t *source);

  /** Gets the source identifier */
  EXPORT const char *ols_source_get_id(const ols_source_t *source);

  /** Returns the signal handler for a source */
  EXPORT signal_handler_t *
  ols_source_get_signal_handler(const ols_source_t *source);

  /** Returns the procedure handler for a source */
  EXPORT proc_handler_t *ols_source_get_proc_handler(const ols_source_t *source);

  /** Returns true if active, false if not */
  EXPORT bool ols_source_active(const ols_source_t *source);

/** Unused flag */
#define OLS_SOURCE_FLAG_UNUSED_1 (1 << 0)
/** Specifies to force audio to mono */
#define OLS_SOURCE_FLAG_FORCE_MONO (1 << 1)

  /**
   * Sets source flags.  Note that these are different from the main output
   * flags.  These are generally things that can be set by the source or user,
   * while the output flags are more used to determine capabilities of a source.
   */
  EXPORT void ols_source_set_flags(ols_source_t *source, uint32_t flags);

  /** Gets source flags. */
  EXPORT uint32_t ols_source_get_flags(const ols_source_t *source);

  /* ------------------------------------------------------------------------- */
  /* Process */
  /**
   * Creates a process of the specified type with the specified settings.
   *
   *   The "process" context is used for anything related to presenting
   * or modifying video/audio.  Use ols_process_release to release it.
   */
  EXPORT ols_process_t *ols_process_create(const char *id, const char *name,
                                           ols_data_t *settings);

  EXPORT ols_process_t *ols_process_create_private(const char *id,
                                                   const char *name,
                                                   ols_data_t *settings);

  /* if source has OLS_SOURCE_DO_NOT_DUPLICATE output flag set, only returns a
   * reference */
  EXPORT ols_process_t *ols_process_duplicate(ols_process_t *process,
                                              const char *desired_name,
                                              bool create_private);
  /**
   * Adds/releases a reference to a source.  When the last reference is
   * released, the source is destroyed.
   */
  OLS_EXTERNAL_DEPRECATED EXPORT void ols_process_addref(ols_process_t *process);
  EXPORT void ols_process_release(ols_process_t *source);

  EXPORT void ols_weak_process_addref(ols_weak_process_t *weak);
  EXPORT void ols_weak_process_release(ols_weak_process_t *weak);

  EXPORT ols_process_t *ols_process_get_ref(ols_process_t *source);
  EXPORT ols_weak_process_t *ols_process_get_weak_process(ols_process_t *process);
  EXPORT ols_process_t *ols_weak_process_get_process(ols_weak_process_t *weak);
  EXPORT bool ols_weak_process_expired(ols_weak_process_t *weak);

  EXPORT bool ols_weak_process_references_procsss(ols_weak_process_t *weak,
                                                  ols_process_t *process);

  /** Notifies all references that the source should be released */
  EXPORT void ols_process_remove(ols_process_t *process);

  /** Returns true if the source should be released */
  EXPORT bool ols_process_removed(const ols_process_t *process);

  /** Gets the default settings for a source type */
  EXPORT ols_data_t *ols_get_process_defaults(const char *id);

  /** Returns the property list, if any.  Free with ols_properties_destroy */
  EXPORT ols_properties_t *ols_get_process_properties(const char *id);

  /** Returns whether the source has custom properties or not */
  EXPORT bool ols_is_process_configurable(const char *id);

  EXPORT bool ols_process_configurable(const ols_process_t *process);

  /**
   * Returns the properties list for a specific existing source.  Free with
   * ols_properties_destroy
   */
  EXPORT ols_properties_t *ols_process_properties(const ols_process_t *process);

  EXPORT void ols_process_reset_settings(ols_process_t *process,
                                         ols_data_t *settings);

  /** Gets the settings string for a source */
  EXPORT ols_data_t *ols_process_get_settings(const ols_process_t *process);

  /** Gets the name of a source */
  EXPORT const char *ols_process_get_name(const ols_process_t *process);

  /** Sets the name of a source */
  EXPORT void ols_process_set_name(ols_process_t *process, const char *name);

  /** Gets the UUID of a source */
  EXPORT const char *ols_process_get_uuid(const ols_process_t *process);

  /** Gets the source type */
  EXPORT enum ols_process_type ols_process_get_type(const ols_process_t *process);

  /** Gets the source identifier */
  EXPORT const char *ols_process_get_id(const ols_process_t *process);

  /** Returns the signal handler for a source */
  EXPORT signal_handler_t *
  ols_process_get_signal_handler(const ols_process_t *process);

  /** Returns the procedure handler for a source */
  EXPORT proc_handler_t *
  ols_process_get_proc_handler(const ols_process_t *process);

  /** Returns true if active, false if not */
  EXPORT bool ols_process_active(const ols_process_t *source);

/** Unused flag */
#define OLS_PROCESS_FLAG_UNUSED_1 (1 << 0)
/** Specifies to force audio to mono */
#define OLS_PROCESS_FLAG_FORCE_MONO (1 << 1)

  /**
   * Sets source flags.  Note that these are different from the main output
   * flags.  These are generally things that can be set by the source or user,
   * while the output flags are more used to determine capabilities of a source.
   */
  EXPORT void ols_process_set_flags(ols_process_t *source, uint32_t flags);

  /** Gets source flags. */
  EXPORT uint32_t ols_process_get_flags(const ols_process_t *source);

  /* ------------------------------------------------------------------------- */
  /* Outputs */

  EXPORT const char *ols_output_get_display_name(const char *id);

  /**
   * Creates an output.
   *
   *   Outputs allow outputting to file, outputting to network, outputting to
   * directshow, or other custom outputs.
   */
  EXPORT ols_output_t *ols_output_create(const char *id, const char *name,
                                         ols_data_t *settings);

  /**
   * Adds/releases a reference to an output.  When the last reference is
   * released, the output is destroyed.
   */
  OLS_EXTERNAL_DEPRECATED EXPORT void ols_output_addref(ols_output_t *output);
  EXPORT void ols_output_release(ols_output_t *output);

  EXPORT void ols_weak_output_addref(ols_weak_output_t *weak);
  EXPORT void ols_weak_output_release(ols_weak_output_t *weak);

  EXPORT ols_output_t *ols_output_get_ref(ols_output_t *output);
  EXPORT ols_weak_output_t *ols_output_get_weak_output(ols_output_t *output);
  EXPORT ols_output_t *ols_weak_output_get_output(ols_weak_output_t *weak);

  EXPORT bool ols_weak_output_references_output(ols_weak_output_t *weak,
                                                ols_output_t *output);

  EXPORT const char *ols_output_get_name(const ols_output_t *output);

  /** Pass a string of the last output error, for UI use */
  EXPORT void ols_output_set_last_error(ols_output_t *output,
                                        const char *message);
  EXPORT const char *ols_output_get_last_error(ols_output_t *output);

/**
 * On reconnection, start where it left of on reconnection.  Note however that
 * this option will consume extra memory to continually increase delay while
 * waiting to reconnect.
 */
#define OLS_OUTPUT_DELAY_PRESERVE (1 << 0)

  /** Gets the default settings for an output type */
  EXPORT ols_data_t *ols_output_defaults(const char *id);

  /** Returns the property list, if any.  Free with ols_properties_destroy */
  EXPORT ols_properties_t *ols_get_output_properties(const char *id);

  /**
   * Returns the property list of an existing output, if any.  Free with
   * ols_properties_destroy
   */
  EXPORT ols_properties_t *ols_output_properties(const ols_output_t *output);

  /* Gets the current output settings string */
  EXPORT ols_data_t *ols_output_get_settings(const ols_output_t *output);

  /** Returns the signal handler for an output  */
  EXPORT signal_handler_t *
  ols_output_get_signal_handler(const ols_output_t *output);

  /** Returns the procedure handler for an output */
  EXPORT proc_handler_t *ols_output_get_proc_handler(const ols_output_t *output);

  /**
   * Sets the reconnect settings.  Set retry_count to 0 to disable reconnecting.
   */
  EXPORT void ols_output_set_reconnect_settings(ols_output_t *output,
                                                int retry_count, int retry_sec);

  /* ------------------------------------------------------------------------- */
  /* Get source icon type */
  EXPORT enum ols_icon_type ols_source_get_icon_type(const char *id);

#ifdef __cplusplus
}
#endif
