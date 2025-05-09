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
#define HASH_FIND_UUID(head, uuid, out) \
  HASH_FIND(hh_uuid, head, uuid, UUID_STR_LENGTH, out)
#define HASH_ADD_UUID(head, uuid_field, add) \
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

/* ------------------------------------------------------------------------- */
/* ols shared context data */

struct ols_weak_ref {
  volatile long refs;
  volatile long weak_refs;
};

struct ols_weak_object {
  struct ols_weak_ref ref;
  struct ols_context_data *object;
};

typedef void (*ols_destroy_cb)(void *obj);

struct ols_context_data {
  char *name;
  const char *uuid;
  void *data;
  ols_data_t *settings;
  signal_handler_t *signals;
  proc_handler_t *procs;
  enum ols_obj_type type;

  struct ols_weak_object *control;
  ols_destroy_cb destroy;

  pthread_mutex_t *mutex;

  /* element pads, these lists can only be iterated while holding
   * the LOCK or checking the cookie after each LOCK. */
  uint16_t numpads;
  DARRAY(ols_pad_t *)
  pads;
  uint16_t numsrcpads;
  DARRAY(ols_pad_t *)
  srcpads;
  uint16_t numsinkpads;
  DARRAY(ols_pad_t *)
  sinkpads;

  UT_hash_handle hh;
  UT_hash_handle hh_uuid;
  bool private;
};

extern bool ols_context_data_init(struct ols_context_data *context,
                                  enum ols_obj_type type, ols_data_t *settings,
                                  const char *name, const char *uuid,
                                  bool private);
extern void ols_context_init_control(struct ols_context_data *context,
                                     void *object, ols_destroy_cb destroy);
extern void ols_context_data_free(struct ols_context_data *context);

extern void ols_context_data_insert_uuid(struct ols_context_data *context,
                                         pthread_mutex_t *mutex,
                                         void *first_uuid);

extern void ols_context_data_remove(struct ols_context_data *context);
extern void ols_context_data_remove_name(struct ols_context_data *context,
                                         void *phead);
extern void ols_context_data_remove_uuid(struct ols_context_data *context,
                                         void *puuid_head);

extern void ols_context_wait(struct ols_context_data *context);

extern void ols_context_data_setname_ht(struct ols_context_data *context,
                                        const char *name, void *phead);

extern bool ols_context_link(struct ols_context_data *src,
                             struct ols_context_data *dest);

extern void ols_context_unlink(struct ols_context_data *src,
                               struct ols_context_data *dest);

extern bool ols_context_link_pads(struct ols_context_data *src,
                                  const char *srcpadname,
                                  struct ols_context_data *dest,
                                  const char *destpadname);

extern void ols_context_unlink_pads(struct ols_context_data *src,
                                    const char *srcpadname,
                                    struct ols_context_data *dest,
                                    const char *destpadname);

/* ------------------------------------------------------------------------- */
/* ref-counting  */

static inline void ols_ref_addref(struct ols_weak_ref *ref) {
  os_atomic_inc_long(&ref->refs);
}

static inline bool ols_ref_release(struct ols_weak_ref *ref) {
  return os_atomic_dec_long(&ref->refs) == -1;
}

static inline void ols_weak_ref_addref(struct ols_weak_ref *ref) {
  os_atomic_inc_long(&ref->weak_refs);
}

static inline bool ols_weak_ref_release(struct ols_weak_ref *ref) {
  return os_atomic_dec_long(&ref->weak_refs) == -1;
}

static inline bool ols_weak_ref_get_ref(struct ols_weak_ref *ref) {
  long owners = os_atomic_load_long(&ref->refs);
  while (owners > -1) {
    if (os_atomic_compare_exchange_long(&ref->refs, &owners, owners + 1)) {
      return true;
    }
  }
  return false;
}

static inline bool ols_weak_ref_expired(struct ols_weak_ref *ref) {
  long owners = os_atomic_load_long(&ref->refs);
  return owners < 0;
}

/* ------------------------------------------------------------------------- */
/* sources  */

struct ols_weak_source {
  struct ols_weak_ref ref;
  struct ols_source *source;
};

struct ols_source {
  struct ols_context_data context;
  struct ols_source_info info;

  ols_pad_t *srcpad;

  /* general exposed flags that can be set for the source */
  uint32_t flags;
  uint32_t default_flags;
  uint32_t last_ols_ver;

  /* indicates ownership of the info.id buffer */
  bool owns_info_id;

  /* signals to call the source update in the video thread */
  long defer_update_count;

  /* ensures activate/deactivate are only called once */
  volatile long activate_refs;

  /* source is in the process of being destroyed */
  volatile long destroying;

  /* used to indicate that the source has been removed and all
   * references to it should be released (not exactly how I would prefer
   * to handle things but it's the best option) */
  bool removed;

  /*  used to indicate if the source should show up when queried for user ui */
  bool temp_removed;

  bool active;

  /* used to temporarily disable sources if needed */
  bool enabled;

  uint64_t last_frame_ts;
  uint64_t last_sys_timestamp;

  /* private data */
  ols_data_t *private_settings;
};

extern struct ols_source_info *get_source_info(const char *id);

extern bool ols_source_init_context(struct ols_source *source,
                                    ols_data_t *settings, const char *name,
                                    const char *uuid, bool private);

extern ols_source_t *ols_source_create_set_last_ver(
    const char *id, const char *name, const char *uuid, ols_data_t *settings,
    uint32_t last_ols_ver, bool is_private);

extern void ols_source_destroy(struct ols_source *source);

static inline void ols_source_dosignal(struct ols_source *source,
                                       const char *signal_ols,
                                       const char *signal_source) {
  struct calldata data;
  uint8_t stack[128];

  calldata_init_fixed(&data, stack, sizeof(stack));
  calldata_set_ptr(&data, "source", source);
  if (signal_ols && !source->context.private)
    signal_handler_signal(ols->signals, signal_ols, &data);
  if (signal_source)
    signal_handler_signal(source->context.signals, signal_source, &data);
}

/* ------------------------------------------------------------------------- */
/* process  */

struct ols_weak_process {
  struct ols_weak_ref ref;
  struct ols_process *process;
};

struct ols_process {
  struct ols_context_data context;
  struct ols_process_info info;

  /* general exposed flags that can be set for the source */
  uint32_t flags;
  uint32_t default_flags;
  uint32_t last_ols_ver;

  /* indicates ownership of the info.id buffer */
  bool owns_info_id;

  /* signals to call the source update in the video thread */
  long defer_update_count;

  /* ensures show/hide are only called once */
  volatile long show_refs;

  /* ensures activate/deactivate are only called once */
  volatile long activate_refs;

  /* source is in the process of being destroyed */
  volatile long destroying;

  /* used to indicate that the source has been removed and all
   * references to it should be released (not exactly how I would prefer
   * to handle things but it's the best option) */
  bool removed;

  bool active;

  uint64_t last_frame_ts;
  uint64_t last_sys_timestamp;

  /* private data */
  /*< protected >*/
  ols_pad_t *sinkpad;
  ols_data_t *private_settings;
};

extern struct ols_process_info *get_process_info(const char *id);

extern bool ols_process_init_context(struct ols_process *source,
                                     ols_data_t *settings, const char *name,
                                     const char *uuid, bool private);

extern ols_process_t *ols_process_create_set_last_ver(
    const char *id, const char *name, const char *uuid, ols_data_t *settings,
    uint32_t last_ols_ver, bool is_private);

extern void ols_process_destroy(struct ols_process *source);

static inline void ols_process_dosignal(struct ols_process *process,
                                        const char *signal_ols,
                                        const char *signal_process) {
  struct calldata data;
  uint8_t stack[128];

  calldata_init_fixed(&data, stack, sizeof(stack));
  calldata_set_ptr(&data, "process", process);
  if (signal_ols && !process->context.private)
    signal_handler_signal(ols->signals, signal_ols, &data);
  if (signal_process)
    signal_handler_signal(process->context.signals, signal_process, &data);
}

/* ------------------------------------------------------------------------- */
/* outputs  */

struct ols_weak_output {
  struct ols_weak_ref ref;
  struct ols_output *output;
};

struct ols_output {
  struct ols_context_data context;
  struct ols_output_info info;
  /* indicates ownership of the info.id buffer */
  bool owns_info_id;
  volatile bool data_active;
  int stop_code;

  int reconnect_retry_sec;
  int reconnect_retry_max;
  int reconnect_retries;
  uint32_t reconnect_retry_cur_msec;
  float reconnect_retry_exp;
  pthread_t reconnect_thread;
  os_event_t *reconnect_stop_event;
  volatile bool reconnecting;
  volatile bool reconnect_thread_active;

  uint32_t starting_drawn_count;
  uint32_t starting_lagged_count;

  ols_pad_t *sinkpad;

  int total_frames;

  bool valid;

  char *last_error_message;
};

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
