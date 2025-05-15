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

#include <inttypes.h>
#include <math.h>

#include "callback/calldata.h"
#include "ols-internal.h"
#include "ols.h"
#include "util/platform.h"
#include "util/threading.h"
#include "util/util_uint64.h"

#define get_weak(process) ((ols_weak_process_t *)process->context.control)

static inline bool data_valid(const struct ols_process *process,
                              const char *f) {
  return ols_process_valid(process, f) && process->context.data;
}

static inline bool destroying(const struct ols_process *process) {
  return os_atomic_load_long(&process->destroying);
}

struct ols_process_info *get_process_info(const char *id) {
  for (size_t i = 0; i < ols->process_types.num; i++) {
    struct ols_process_info *info = &ols->process_types.array[i];
    if (strcmp(info->id, id) == 0) return info;
  }

  return NULL;
}

static const char *process_signals[] = {
    "void destroy(ptr process)",
    "void remove(ptr process)",
    "void update(ptr process)",
    "void save(ptr process)",
    "void load(ptr process)",
    "void activate(ptr process)",
    "void deactivate(ptr process)",
    "void enable(ptr process, bool enabled)",
    "void update_properties(ptr process)",
    "void update_flags(ptr process, int flags)",
    NULL,
};

static OlsFlowReturn process_default_chain(ols_pad_t *pad, ols_object_t *parent,
                                           ols_buffer_t *buffer) {
  struct ols_process *process = (struct ols_process *)parent;
  UNUSED_PARAMETER(pad);
  // process->info.analysis(buffer);

  for (int i = 0; i < process->context.numsrcpads; ++i) {
    ols_pad_t *pad = process->context.srcpads.array[i];

    ols_pad_push(pad, buffer);
  }
}

bool ols_process_init_context(struct ols_process *process, ols_data_t *settings,
                              const char *name, const char *uuid,
                              bool private) {
  if (!ols_context_data_init(&process->context, OLS_OBJ_TYPE_PROCESS, settings,
                             name, uuid, private))
    return false;

  // process->context.sink =
  //     gst_ghost_pad_new_from_template("sink", scale->sinkpads->data,
  //     template);
  ols_pad_set_chain_function(process->sinkpad,
                             (ols_pad_chain_function)process_default_chain);
  // gst_element_add_pad(GST_ELEMENT(self), process->sink);
  // gst_object_unref(template);

  // gst_element_add_pad(GST_ELEMENT(self), pad);

  return signal_handler_add_array(process->context.signals, process_signals);
}

const char *ols_process_get_display_name(const char *id) {
  const struct ols_process_info *info = get_process_info(id);
  return (info != NULL) ? info->get_name(info->type_data) : NULL;
}

/* internal initialization */
static bool ols_process_init(struct ols_process *process) {
  ols_context_init_control(&process->context, process,
                           (ols_destroy_cb)ols_process_destroy);

  process->private_settings = ols_data_create();
  return true;
}

static void ols_process_init_finalize(struct ols_process *process) {
  ols_context_data_insert_uuid(&process->context, &ols->data.processes_mutex,
                               &ols->data.processes);
}

static ols_process_t *ols_process_create_internal(
    const char *id, const char *name, const char *uuid, ols_data_t *settings,
    bool private, uint32_t last_ols_ver) {
  struct ols_process *process = bzalloc(sizeof(struct ols_process));

  const struct ols_process_info *info = get_process_info(id);
  if (!info) {
    blog(LOG_ERROR, "Process ID '%s' not found", id);

    process->info.id = bstrdup(id);
    process->owns_info_id = true;
  } else {
    process->info = *info;
  }

  process->last_ols_ver = last_ols_ver;

  if (!ols_process_init_context(process, settings, name, uuid, private))
    goto fail;

  if (info) {
    if (info->get_defaults) {
      info->get_defaults(process->context.settings);
    }
  }

  if (!ols_process_init(process)) goto fail;

  /* allow the process to be created even if creation fails so that the
   * user's data doesn't become lost */
  if (info && info->create)
    process->context.data = info->create(process->context.settings, process);
  if ((!info || info->create) && !process->context.data)
    blog(LOG_ERROR, "Failed to create process '%s'!", name);

  blog(LOG_DEBUG, "%sprocess '%s' (%s) created", private ? "private " : "",
       name, id);

  process->flags = process->default_flags;
  // process->enabled = true;

  ols_process_init_finalize(process);
  if (!private) {
    ols_process_dosignal(process, "process_create", NULL);
  }

  return process;

fail:
  blog(LOG_ERROR, "ols_process_create failed");
  ols_process_destroy(process);
  return NULL;
}

ols_process_t *ols_process_create(const char *id, const char *name,
                                  ols_data_t *settings) {
  return ols_process_create_internal(id, name, NULL, settings, false,
                                     LIBOLS_API_VER);
}

ols_process_t *ols_process_create_private(const char *id, const char *name,
                                          ols_data_t *settings) {
  return ols_process_create_internal(id, name, NULL, settings, true,
                                     LIBOLS_API_VER);
}

ols_process_t *ols_process_create_set_last_ver(const char *id, const char *name,
                                               const char *uuid,
                                               ols_data_t *settings,
                                               uint32_t last_ols_ver,
                                               bool is_private) {
  return ols_process_create_internal(id, name, uuid, settings, is_private,
                                     last_ols_ver);
}

ols_process_t *ols_process_duplicate(ols_process_t *process,
                                     const char *new_name,
                                     bool create_private) {
  ols_process_t *new_process;
  ols_data_t *settings;

  if (!ols_process_valid(process, "ols_process_duplicate")) return NULL;

  if ((process->info.output_flags & OLS_PROCESS_DO_NOT_DUPLICATE) != 0) {
    return ols_process_get_ref(process);
  }

  settings = ols_data_create();
  ols_data_apply(settings, process->context.settings);

  new_process =
      create_private
          ? ols_process_create_private(process->info.id, new_name, settings)
          : ols_process_create(process->info.id, new_name, settings);

  new_process->flags = process->flags;

  ols_data_apply(new_process->private_settings, process->private_settings);

  ols_data_release(settings);
  return new_process;
}

static void ols_process_destroy_defer(struct ols_process *process);

void ols_process_destroy(struct ols_process *process) {
  if (!ols_process_valid(process, "ols_process_destroy")) return;

  if (os_atomic_set_long(&process->destroying, true) == true) {
    blog(LOG_ERROR,
         "Double destroy just occurred. "
         "Something called addref on a process "
         "after it was already fully released, "
         "I guess.");
    return;
  }

  // ols_context_data_remove_uuid(&process->context, &ols->data.processs);
  // if (!process->context.private)
  //   ols_context_data_remove_name(&process->context,
  //   &ols->data.public_processs);

  /* defer process destroy */
  os_task_queue_queue_task(ols->destruction_task_thread,
                           (os_task_t)ols_process_destroy_defer, process);
}

void ols_process_destroy_defer(struct ols_process *process) {
  size_t i;

  /* prevents the destruction of processs if destroy triggered inside of
   * a video tick call */
  ols_context_wait(&process->context);

  ols_process_dosignal(process, "process_destroy", "destroy");

  if (process->context.data) {
    process->info.destroy(process->context.data);
    process->context.data = NULL;
  }

  blog(LOG_DEBUG, "%sprocess '%s' destroyed",
       process->context.private ? "private " : "", process->context.name);

  ols_data_release(process->private_settings);
  ols_context_data_free(&process->context);

  if (process->owns_info_id) {
    bfree((void *)process->info.id);
  }

  bfree(process);
}

void ols_process_addref(ols_process_t *process) {
  if (!process) return;

  ols_ref_addref(&process->context.control->ref);
}

void ols_process_release(ols_process_t *process) {
  if (!ols && process) {
    blog(LOG_WARNING,
         "Tried to release a process when the OLS "
         "core is shut down!");
    return;
  }

  if (!process) return;

  ols_weak_process_t *control = get_weak(process);
  if (ols_ref_release(&control->ref)) {
    ols_process_destroy(process);
    ols_weak_process_release(control);
  }
}

void ols_weak_process_addref(ols_weak_process_t *weak) {
  if (!weak) return;

  ols_weak_ref_addref(&weak->ref);
}

void ols_weak_process_release(ols_weak_process_t *weak) {
  if (!weak) return;

  if (ols_weak_ref_release(&weak->ref)) bfree(weak);
}

ols_process_t *ols_process_get_ref(ols_process_t *process) {
  if (!process) return NULL;

  return ols_weak_process_get_process(get_weak(process));
}

ols_weak_process_t *ols_process_get_weak_process(ols_process_t *process) {
  if (!process) return NULL;

  ols_weak_process_t *weak = get_weak(process);
  ols_weak_process_addref(weak);
  return weak;
}

ols_process_t *ols_weak_process_get_process(ols_weak_process_t *weak) {
  if (!weak) return NULL;

  if (ols_weak_ref_get_ref(&weak->ref)) return weak->process;

  return NULL;
}

bool ols_weak_process_expired(ols_weak_process_t *weak) {
  return weak ? ols_weak_ref_expired(&weak->ref) : true;
}

bool ols_weak_process_references_process(ols_weak_process_t *weak,
                                         ols_process_t *process) {
  return weak && process && weak->process == process;
}

void ols_process_remove(ols_process_t *process) {
  if (!ols_process_valid(process, "ols_process_remove")) return;

  if (!process->removed) {
    ols_process_t *s = ols_process_get_ref(process);
    if (s) {
      s->removed = true;
      ols_process_dosignal(s, "process_remove", "remove");
      ols_process_release(s);
    }
  }
}

bool ols_process_removed(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_removed") ? process->removed
                                                           : true;
}

static inline ols_data_t *get_defaults(const struct ols_process_info *info) {
  ols_data_t *settings = ols_data_create();
  if (info->get_defaults) info->get_defaults(settings);
  return settings;
}

ols_data_t *ols_process_settings(const char *id) {
  const struct ols_process_info *info = get_process_info(id);
  return (info) ? get_defaults(info) : NULL;
}

ols_data_t *ols_get_process_defaults(const char *id) {
  const struct ols_process_info *info = get_process_info(id);
  return info ? get_defaults(info) : NULL;
}

ols_properties_t *ols_get_process_properties(const char *id) {
  const struct ols_process_info *info = get_process_info(id);
  if (info && info->get_properties) {
    ols_data_t *defaults = get_defaults(info);
    ols_properties_t *props;

    props = info->get_properties(NULL);

    ols_properties_apply_settings(props, defaults);
    ols_data_release(defaults);
    return props;
  }
  return NULL;
}

bool ols_is_process_configurable(const char *id) {
  const struct ols_process_info *info = get_process_info(id);
  return info && (info->get_properties);
}

bool ols_process_configurable(const ols_process_t *process) {
  return data_valid(process, "ols_process_configurable") &&
         (process->info.get_properties);
}

ols_properties_t *ols_process_properties(const ols_process_t *process) {
  if (!data_valid(process, "ols_process_properties")) return NULL;

  if (process->info.get_properties) {
    ols_properties_t *props;
    props = process->info.get_properties(process->context.data);
    ols_properties_apply_settings(props, process->context.settings);
    return props;
  }

  return NULL;
}

void ols_process_update(ols_process_t *process, ols_data_t *settings) {
  if (!ols_process_valid(process, "ols_process_update")) return;

  if (settings) {
    ols_data_apply(process->context.settings, settings);
  }

  if (process->context.data && process->info.update) {
    process->info.update(process->context.data, process->context.settings);
    ols_process_dosignal(process, "process_update", "update");
  }
}

void ols_process_reset_settings(ols_process_t *process, ols_data_t *settings) {
  if (!ols_process_valid(process, "ols_process_reset_settings")) return;

  ols_data_clear(process->context.settings);
  ols_process_update(process, settings);
}

void ols_process_update_properties(ols_process_t *process) {
  if (!ols_process_valid(process, "ols_process_update_properties")) return;

  ols_process_dosignal(process, NULL, "update_properties");
}

static void activate_process(ols_process_t *process) {
  if (process->context.data && process->info.activate)
    process->info.activate(process->context.data);
  ols_process_dosignal(process, "process_activate", "activate");
}

static void deactivate_process(ols_process_t *process) {
  if (process->context.data && process->info.deactivate)
    process->info.deactivate(process->context.data);
  ols_process_dosignal(process, "process_deactivate", "deactivate");
}

static inline uint64_t uint64_diff(uint64_t ts1, uint64_t ts2) {
  return (ts1 < ts2) ? (ts2 - ts1) : (ts1 - ts2);
}

ols_data_t *ols_process_get_settings(const ols_process_t *process) {
  if (!ols_process_valid(process, "ols_process_get_settings")) return NULL;

  ols_data_addref(process->context.settings);
  return process->context.settings;
}

const char *ols_process_get_name(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_get_name")
             ? process->context.name
             : NULL;
}

const char *ols_process_get_uuid(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_get_uuid")
             ? process->context.uuid
             : NULL;
}

void ols_process_set_name(ols_process_t *process, const char *name) {
  if (!ols_process_valid(process, "ols_process_set_name")) return;

  if (!name || !*name || !process->context.name ||
      strcmp(name, process->context.name) != 0) {
    struct calldata data;
    char *prev_name = bstrdup(process->context.name);

    if (!process->context.private) {
      // ols_context_data_setname_ht(&process->context, name,
      //                             &ols->data.public_processs);
    }
    calldata_init(&data);
    calldata_set_ptr(&data, "process", process);
    calldata_set_string(&data, "new_name", process->context.name);
    calldata_set_string(&data, "prev_name", prev_name);
    if (!process->context.private)
      signal_handler_signal(ols->signals, "process_rename", &data);
    signal_handler_signal(process->context.signals, "rename", &data);
    calldata_free(&data);
    bfree(prev_name);
  }
}

enum ols_process_type ols_process_get_type(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_get_type")
             ? process->info.type
             : OLS_PROCESS_TYPE_INPUT;
}

const char *ols_process_get_id(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_get_id") ? process->info.id
                                                          : NULL;
}

signal_handler_t *ols_process_get_signal_handler(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_get_signal_handler")
             ? process->context.signals
             : NULL;
}

proc_handler_t *ols_process_get_proc_handler(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_get_proc_handler")
             ? process->context.procs
             : NULL;
}

void ols_process_save(ols_process_t *process) {
  if (!data_valid(process, "ols_process_save")) return;

  ols_process_dosignal(process, "process_save", "save");

  if (process->info.save)
    process->info.save(process->context.data, process->context.settings);
}

void ols_process_load(ols_process_t *process) {
  if (!data_valid(process, "ols_process_load")) return;
  if (process->info.load)
    process->info.load(process->context.data, process->context.settings);

  ols_process_dosignal(process, "process_load", "load");
}

bool ols_process_active(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_active")
             ? process->activate_refs != 0
             : false;
}

bool ols_process_showing(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_showing")
             ? process->show_refs != 0
             : false;
}

static inline void signal_flags_updated(ols_process_t *process) {
  struct calldata data;
  uint8_t stack[128];

  calldata_init_fixed(&data, stack, sizeof(stack));
  calldata_set_ptr(&data, "process", process);
  calldata_set_int(&data, "flags", process->flags);

  signal_handler_signal(process->context.signals, "update_flags", &data);
}

void ols_process_set_flags(ols_process_t *process, uint32_t flags) {
  if (!ols_process_valid(process, "ols_process_set_flags")) return;

  if (flags != process->flags) {
    process->flags = flags;
    signal_flags_updated(process);
  }
}

void ols_process_set_default_flags(ols_process_t *process, uint32_t flags) {
  if (!ols_process_valid(process, "ols_process_set_default_flags")) return;

  process->default_flags = flags;
}

uint32_t ols_process_get_flags(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_get_flags") ? process->flags
                                                             : 0;
}

ols_data_t *ols_process_get_private_settings(ols_process_t *process) {
  if (!ols_ptr_valid(process, "ols_process_get_private_settings")) return NULL;

  ols_data_addref(process->private_settings);
  return process->private_settings;
}

uint32_t ols_process_get_last_ols_version(const ols_process_t *process) {
  return ols_process_valid(process, "ols_process_get_last_ols_version")
             ? process->last_ols_ver
             : 0;
}
