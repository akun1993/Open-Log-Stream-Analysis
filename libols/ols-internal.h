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
#include <stdbool.h>
#include "callback/proc.h"
#include "callback/signal.h"
#include "ols.h"
#include "util/c99defs.h"
#include "util/darray.h"
#include "util/deque.h"
#include "util/dstr.h"
#include "util/platform.h"
#include "util/profiler.h"
#include "util/task.h"
#include "util/threading.h"
#include "util/uthash.h"

/* Custom helpers for the UUID hash table */
#define HASH_FIND_UUID(head, uuid, out)                                        \
  HASH_FIND(hh_uuid, head, uuid, UUID_STR_LENGTH, out)
#define HASH_ADD_UUID(head, uuid_field, add)                                   \
  HASH_ADD(hh_uuid, head, uuid_field[0], UUID_STR_LENGTH, add)

struct tick_callback {
  void (*tick)(void *param, float seconds);
  void *param;
};

/* ------------------------------------------------------------------------- */
/* validity checks */

static inline bool ols_object_valid(const void *obj, const char *f,
                                    const char *t) {
  if (!obj) {
    blog(LOG_DEBUG, "%s: Null '%s' parameter", f, t);
    return false;
  }

  return true;
}

#define ols_ptr_valid(ptr, func) ols_object_valid(ptr, func, #ptr)
#define ols_source_valid ols_ptr_valid
#define ols_output_valid ols_ptr_valid
#define ols_process_valid ols_ptr_valid

/* ------------------------------------------------------------------------- */
/* modules */

struct ols_module {
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

struct ols_module_path {
  char *bin;
  char *data;
};

static inline void free_module_path(struct ols_module_path *omp) {
  if (omp) {
    bfree(omp->bin);
    bfree(omp->data);
  }
}

static inline bool check_path(const char *data, const char *path,
                              struct dstr *output) {
  dstr_copy(output, path);
  dstr_cat(output, data);

  return os_file_exists(output->array);
}

/* ------------------------------------------------------------------------- */
/* core */

struct ols_task_info {
  ols_ev_task_t task;
  void *param;
};

/* user sources, output channels, and displays */
struct ols_core_data {
  /* Hash tables (uthash) */
  struct ols_source *sources;        /* Lookup by UUID (hh_uuid) */
  struct ols_source *public_sources; /* Lookup by name (hh) */
  struct ols_process *processes;     /* Lookup by UUID (hh_uuid) */
  

  /* Linked lists */
  struct ols_output *first_output;

  pthread_mutex_t sources_mutex;
  pthread_mutex_t processes_mutex;
  pthread_mutex_t outputs_mutex;
  DARRAY(struct tick_callback)
  tick_callbacks;

  // struct ols_view main_view;

  long long unnamed_index;

  ols_data_t *private_data;

  volatile bool valid;

  DARRAY(char *)
  protocols;
  DARRAY(ols_source_t *)
  sources_to_tick;
};

typedef DARRAY(struct ols_source_info) ols_source_info_array_t;

struct ols_core {
  struct ols_module *first_module;
  DARRAY(struct ols_module_path)
  module_paths;
  DARRAY(char *)
  safe_modules;

  ols_source_info_array_t source_types;
  // ols_source_info_array_t input_types;
  // ols_source_info_array_t filter_types;
  DARRAY(struct ols_process_info)
  process_types;
  DARRAY(struct ols_output_info)
  output_types;

  signal_handler_t *signals;
  proc_handler_t *procs;

  char *locale;
  char *module_config_path;
  bool name_store_owned;
  profiler_name_store_t *name_store;

  /* segmented into multiple sub-structures to keep things a bit more
   * clean and organized */
  struct ols_core_data data;
  os_task_queue_t *destruction_task_thread;
  ols_ev_task_handler_t ui_task_handler;
};

extern struct ols_core *ols;


//************************************* */
// source 
extern struct ols_source_info *get_source_info(const char *id);
  
extern bool ols_source_init_context(struct ols_source *source,
                  ols_data_t *settings, const char *name,
                  const char *uuid);

extern ols_source_t *
ols_source_create_set_last_ver(const char *id, const char *name,
               const char *uuid, ols_data_t *settings,
               uint32_t last_ols_ver);

extern void ols_source_destroy(struct ols_source *source);

static inline void ols_source_dosignal(struct ols_source *source,
                   const char *signal_ols,
                   const char *signal_source) {
struct calldata data;
uint8_t stack[128];

calldata_init_fixed(&data, stack, sizeof(stack));
calldata_set_ptr(&data, "source", source);
if (signal_ols )
  signal_handler_signal(ols->signals, signal_ols, &data);
if (signal_source)
  signal_handler_signal(source->context.signals, signal_source, &data);
}


//************************************* */
// process 
extern struct ols_process_info *get_process_info(const char *id);

extern bool ols_process_init_context(struct ols_process *source,
                                     ols_data_t *settings, const char *name,
                                     const char *uuid);

extern ols_process_t *
ols_process_create_set_last_ver(const char *id, const char *name,
                                const char *uuid, ols_data_t *settings,
                                uint32_t last_ols_ver);

extern void ols_process_destroy(struct ols_process *source);

static inline void ols_process_dosignal(struct ols_process *process,
                                        const char *signal_ols,
                                        const char *signal_process) {
  struct calldata data;
  uint8_t stack[128];

  calldata_init_fixed(&data, stack, sizeof(stack));
  calldata_set_ptr(&data, "process", process);
  if (signal_ols )
    signal_handler_signal(ols->signals, signal_ols, &data);
  if (signal_process)
    signal_handler_signal(process->context.signals, signal_process, &data);
}

//************************************* */
// output 
static inline void do_output_signal(struct ols_output *output,
  const char *signal) {
struct calldata params = {0};
calldata_set_ptr(&params, "output", output);
signal_handler_signal(output->context.signals, signal, &params);
calldata_free(&params);
}

extern bool ols_output_actual_start(ols_output_t *output);
extern void ols_output_actual_stop(ols_output_t *output, bool force,
 uint64_t ts);

extern const struct ols_output_info *find_output(const char *id);

void ols_output_destroy(ols_output_t *output);
