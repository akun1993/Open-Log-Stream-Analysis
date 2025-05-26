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

#include "callback/calldata.h"
#include "ols-internal.h"
#include "ols.h"
#include "util/platform.h"
#include "util/threading.h"
#include "util/util_uint64.h"
#include <inttypes.h>
#include <math.h>

#define get_weak(source) ((ols_weak_source_t *)source->context.control)

static inline bool data_valid(const struct ols_source *source, const char *f) {
  return ols_source_valid(source, f) && source->context.data;
}

static inline bool destroying(const struct ols_source *source) {
  return os_atomic_load_long(&source->destroying);
}

struct ols_source_info *get_source_info(const char *id) {
  for (size_t i = 0; i < ols->source_types.num; i++) {
    struct ols_source_info *info = &ols->source_types.array[i];
    if (strcmp(info->id, id) == 0)
      return info;
  }

  return NULL;
}

static const char *source_signals[] = {
    "void destroy(ptr source)",
    "void remove(ptr source)",
    "void update(ptr source)",
    "void save(ptr source)",
    "void load(ptr source)",
    "void activate(ptr source)",
    "void deactivate(ptr source)",
    "void enable(ptr source, bool enabled)",
    "void update_properties(ptr source)",
    "void update_flags(ptr source, int flags)",
    NULL,
};

bool ols_source_init_context(struct ols_source *source, ols_data_t *settings,
                             const char *name, const char *uuid, bool private) {
  if (!ols_context_data_init(&source->context, OLS_OBJ_TYPE_SOURCE, settings,
                             name, uuid, private))
    return false;

  return signal_handler_add_array(source->context.signals, source_signals);
}

const char *ols_source_get_display_name(const char *id) {
  const struct ols_source_info *info = get_source_info(id);
  return (info != NULL) ? info->get_name(info->type_data) : NULL;
}

extern char *find_libols_data_file(const char *file);

/* internal initialization */
static bool ols_source_init(struct ols_source *source) {

  ols_context_init_control(&source->context, source,
                           (ols_destroy_cb)ols_source_destroy);

  ols_pad_t *srcpad = ols_pad_new("src", OLS_PAD_SRC);

  // ols_pad_set_link_function(source->srcpad, base_src_link_func);
  // ols_pad_set_chain_function(source->srcpad, base_src_chain_func);

  source->srcpad = srcpad;

  blog(LOG_DEBUG, "ols_context_add_pad");
  ols_context_add_pad(&source->context, source->srcpad);

  source->private_settings = ols_data_create();
  return true;
}

static void ols_source_init_finalize(struct ols_source *source) {
  if (!source->context.is_private) {
    ols_context_data_insert_name(&source->context, &ols->data.sources_mutex,
                                 &ols->data.public_sources);
  }

  ols_context_data_insert_uuid(&source->context, &ols->data.sources_mutex,
                               &ols->data.sources);
}

static ols_source_t *
ols_source_create_internal(const char *id, const char *name, const char *uuid,
                           ols_data_t *settings, bool private,
                           uint32_t last_ols_ver) {
  struct ols_source *source = bzalloc(sizeof(struct ols_source));

  const struct ols_source_info *info = get_source_info(id);
  if (!info) {
    blog(LOG_ERROR, "Source ID '%s' not found", id);

    source->info.id = bstrdup(id);
    source->owns_info_id = true;
  } else {
    source->info = *info;
  }

  source->last_ols_ver = last_ols_ver;

  if (!ols_source_init_context(source, settings, name, uuid, private))
    goto fail;

  if (info) {
    if (info->get_defaults) {
      info->get_defaults(source->context.settings);
    }
  }

  if (!ols_source_init(source))
    goto fail;

  /* allow the source to be created even if creation fails so that the
   * user's data doesn't become lost */
  if (info && info->create)
    source->context.data = info->create(source->context.settings, source);
  if ((!info || info->create) && !source->context.data)
    blog(LOG_ERROR, "Failed to create source '%s'!", name);

  blog(LOG_DEBUG, "%ssource '%s' (%s) created", private ? "private " : "", name,
       id);

  source->flags = source->default_flags;
  source->enabled = true;

  ols_source_init_finalize(source);
  if (!private) {
    ols_source_dosignal(source, "source_create", NULL);
  }

  return source;

fail:
  blog(LOG_ERROR, "ols_source_create failed");
  ols_source_destroy(source);
  return NULL;
}

ols_source_t *ols_source_create(const char *id, const char *name,
                                ols_data_t *settings) {
  return ols_source_create_internal(id, name, NULL, settings, false,
                                    LIBOLS_API_VER);
}

ols_source_t *ols_source_create_private(const char *id, const char *name,
                                        ols_data_t *settings) {
  return ols_source_create_internal(id, name, NULL, settings, true,
                                    LIBOLS_API_VER);
}

ols_source_t *ols_source_create_set_last_ver(const char *id, const char *name,
                                             const char *uuid,
                                             ols_data_t *settings,
                                             uint32_t last_ols_ver,
                                             bool is_private) {
  return ols_source_create_internal(id, name, uuid, settings, is_private,
                                    last_ols_ver);
}

ols_source_t *ols_source_duplicate(ols_source_t *source, const char *new_name,
                                   bool create_private) {
  ols_source_t *new_source;
  ols_data_t *settings;

  if (!ols_source_valid(source, "ols_source_duplicate"))
    return NULL;

  if ((source->info.output_flags & OLS_SOURCE_DO_NOT_DUPLICATE) != 0) {
    return ols_source_get_ref(source);
  }

  settings = ols_data_create();
  ols_data_apply(settings, source->context.settings);

  new_source =
      create_private
          ? ols_source_create_private(source->info.id, new_name, settings)
          : ols_source_create(source->info.id, new_name, settings);

  new_source->flags = source->flags;

  ols_data_apply(new_source->private_settings, source->private_settings);

  ols_data_release(settings);
  return new_source;
}

static void ols_source_destroy_defer(struct ols_source *source);

void ols_source_destroy(struct ols_source *source) {
  if (!ols_source_valid(source, "ols_source_destroy"))
    return;

  if (os_atomic_set_long(&source->destroying, true) == true) {
    blog(LOG_ERROR, "Double destroy just occurred. "
                    "Something called addref on a source "
                    "after it was already fully released, "
                    "I guess.");
    return;
  }

  ols_context_data_remove_uuid(&source->context, &ols->data.sources);
  if (!source->context.is_private)
    ols_context_data_remove_name(&source->context, &ols->data.public_sources);

  /* defer source destroy */
  os_task_queue_queue_task(ols->destruction_task_thread,
                           (os_task_t)ols_source_destroy_defer, source);
}

void ols_source_destroy_defer(struct ols_source *source) {
  size_t i;

  /* prevents the destruction of sources if destroy triggered inside of
   * a video tick call */
  ols_context_wait(&source->context);

  ols_source_dosignal(source, "source_destroy", "destroy");

  if (source->context.data) {
    source->info.destroy(source->context.data);
    source->context.data = NULL;
  }

  blog(LOG_DEBUG, "%ssource '%s' destroyed",
       source->context.is_private ? "private " : "", source->context.name);

  ols_data_release(source->private_settings);
  ols_context_data_free(&source->context);

  if (source->owns_info_id) {
    bfree((void *)source->info.id);
  }

  bfree(source);
}

void ols_source_addref(ols_source_t *source) {
  if (!source)
    return;

  ols_ref_addref(&source->context.control->ref);
}

void ols_source_release(ols_source_t *source) {
  if (!ols && source) {
    blog(LOG_WARNING, "Tried to release a source when the OLS "
                      "core is shut down!");
    return;
  }

  if (!source)
    return;

  ols_weak_source_t *control = get_weak(source);
  if (ols_ref_release(&control->ref)) {
    ols_source_destroy(source);
    ols_weak_source_release(control);
  }
}

void ols_weak_source_addref(ols_weak_source_t *weak) {
  if (!weak)
    return;

  ols_weak_ref_addref(&weak->ref);
}

void ols_weak_source_release(ols_weak_source_t *weak) {
  if (!weak)
    return;

  if (ols_weak_ref_release(&weak->ref))
    bfree(weak);
}

ols_source_t *ols_source_get_ref(ols_source_t *source) {
  if (!source)
    return NULL;

  return ols_weak_source_get_source(get_weak(source));
}

ols_weak_source_t *ols_source_get_weak_source(ols_source_t *source) {
  if (!source)
    return NULL;

  ols_weak_source_t *weak = get_weak(source);
  ols_weak_source_addref(weak);
  return weak;
}

ols_source_t *ols_weak_source_get_source(ols_weak_source_t *weak) {
  if (!weak)
    return NULL;

  if (ols_weak_ref_get_ref(&weak->ref))
    return weak->source;

  return NULL;
}

bool ols_weak_source_expired(ols_weak_source_t *weak) {
  return weak ? ols_weak_ref_expired(&weak->ref) : true;
}

bool ols_weak_source_references_source(ols_weak_source_t *weak,
                                       ols_source_t *source) {
  return weak && source && weak->source == source;
}

void ols_source_remove(ols_source_t *source) {
  if (!ols_source_valid(source, "ols_source_remove"))
    return;

  if (!source->removed) {
    ols_source_t *s = ols_source_get_ref(source);
    if (s) {
      s->removed = true;
      ols_source_dosignal(s, "source_remove", "remove");
      ols_source_release(s);
    }
  }
}

bool ols_source_removed(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_removed") ? source->removed
                                                        : true;
}

static inline ols_data_t *get_defaults(const struct ols_source_info *info) {
  ols_data_t *settings = ols_data_create();
  if (info->get_defaults)
    info->get_defaults(settings);
  return settings;
}

ols_data_t *ols_source_settings(const char *id) {
  const struct ols_source_info *info = get_source_info(id);
  return (info) ? get_defaults(info) : NULL;
}

ols_data_t *ols_get_source_defaults(const char *id) {
  const struct ols_source_info *info = get_source_info(id);
  return info ? get_defaults(info) : NULL;
}

ols_properties_t *ols_get_source_properties(const char *id) {
  const struct ols_source_info *info = get_source_info(id);
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

bool ols_is_source_configurable(const char *id) {
  const struct ols_source_info *info = get_source_info(id);
  return info && (info->get_properties);
}

bool ols_source_configurable(const ols_source_t *source) {
  return data_valid(source, "ols_source_configurable") &&
         (source->info.get_properties);
}

ols_properties_t *ols_source_properties(const ols_source_t *source) {
  if (!data_valid(source, "ols_source_properties"))
    return NULL;

  if (source->info.get_properties) {
    ols_properties_t *props;
    props = source->info.get_properties(source->context.data);
    ols_properties_apply_settings(props, source->context.settings);
    return props;
  }

  return NULL;
}

void ols_source_update(ols_source_t *source, ols_data_t *settings) {
  if (!ols_source_valid(source, "ols_source_update"))
    return;

  if (settings) {
    ols_data_apply(source->context.settings, settings);
  }

  if (source->context.data && source->info.update) {
    source->info.update(source->context.data, source->context.settings);
    ols_source_dosignal(source, "source_update", "update");
  }
}

void ols_source_reset_settings(ols_source_t *source, ols_data_t *settings) {
  if (!ols_source_valid(source, "ols_source_reset_settings"))
    return;

  ols_data_clear(source->context.settings);
  ols_source_update(source, settings);
}

void ols_source_update_properties(ols_source_t *source) {
  if (!ols_source_valid(source, "ols_source_update_properties"))
    return;

  ols_source_dosignal(source, NULL, "update_properties");
}

static void activate_source(ols_source_t *source) {
  if (source->context.data && source->info.activate)
    source->info.activate(source->context.data);
  ols_source_dosignal(source, "source_activate", "activate");
}

static void deactivate_source(ols_source_t *source) {
  if (source->context.data && source->info.deactivate)
    source->info.deactivate(source->context.data);
  ols_source_dosignal(source, "source_deactivate", "deactivate");
}

static int ols_src_get_stream_data(ols_source_t *source, ols_buffer_t *buf) {

  if (source->context.data && source->info.get_data) {
    return source->info.get_data(source->context.data, buf);
  } else {
    return OLS_FLOW_EOS;
  }
}

static void ols_base_src_loop(ols_pad_t *pad) {
  ols_source_t *src;

  src = (ols_source_t *)(OLS_PAD_PARENT(pad));

  // base_src_send_stream_start (src);
  ols_buffer_t *buf = (ols_buffer_t *)bmalloc(sizeof(ols_buffer_t));
  int ret = ols_src_get_stream_data(src, buf);
  if ((ret != OLS_FLOW_OK)) {
    goto pause;
  }
  /* this should not happen */
  if ((buf == NULL))
    goto null_buffer;

  ret = ols_pad_push(pad, buf);
  if ((ret != OLS_FLOW_OK)) {

    blog(LOG_WARNING, "pad push buf failed , ret = %d", ret);
    // goto pause;
  }
  /* Segment pending means that a new segment was configured
   * during this loop run */
done:
  return;

pause: {
  ols_event_t *event;
  // src->running = false;
  ols_pad_pause_task(pad);
  if (ret == OLS_FLOW_EOS) {

    event = ols_event_new_eos();
    // gst_event_set_seqnum (event, src->priv->seqnum);

    ols_pad_push_event(pad, event);
    // src->priv->forced_eos = FALSE;
  }
  goto done;
}
null_buffer: {
  blog(LOG_ERROR, "Internal data flow error, element returned NULL buffer");
  goto done;
}
}

// /** Set this source to be active or deactive */
bool ols_source_set_active(ols_source_t *source, bool is_active) {

  if (is_active) {
    if (!source->active) {
      activate_source(source);

      if (source->srcpad) {
        ols_pad_start_task(source->srcpad, (os_task_t)ols_base_src_loop,
                           (void *)source->srcpad);
      } else {
        blog(LOG_ERROR, "source '%s' has no src pad", source->context.name);
      }

    } else {
      blog(LOG_WARNING, "source '%s' do not repate active",
           source->context.name);
    }
  } else {

    if (source->active) {

      if (source->srcpad) {
        ols_pad_pause_task(source->srcpad);
      } else {
        blog(LOG_ERROR, "source '%s' has no src pad", source->context.name);
      }

      deactivate_source(source);
    } else {
      blog(LOG_WARNING, "source '%s' do not repate deactive",
           source->context.name);
    }
  }

  source->active = is_active;
}

static inline uint64_t uint64_diff(uint64_t ts1, uint64_t ts2) {
  return (ts1 < ts2) ? (ts2 - ts1) : (ts1 - ts2);
}

ols_data_t *ols_source_get_settings(const ols_source_t *source) {
  if (!ols_source_valid(source, "ols_source_get_settings"))
    return NULL;

  ols_data_addref(source->context.settings);
  return source->context.settings;
}

const char *ols_source_get_name(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_get_name") ? source->context.name
                                                         : NULL;
}

const char *ols_source_get_uuid(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_get_uuid") ? source->context.uuid
                                                         : NULL;
}

void ols_source_set_name(ols_source_t *source, const char *name) {
  if (!ols_source_valid(source, "ols_source_set_name"))
    return;

  if (!name || !*name || !source->context.name ||
      strcmp(name, source->context.name) != 0) {
    struct calldata data;
    char *prev_name = bstrdup(source->context.name);

    if (!source->context.is_private) {
      ols_context_data_setname_ht(&source->context, name,
                                  &ols->data.public_sources);
    }
    calldata_init(&data);
    calldata_set_ptr(&data, "source", source);
    calldata_set_string(&data, "new_name", source->context.name);
    calldata_set_string(&data, "prev_name", prev_name);
    if (!source->context.is_private)
      signal_handler_signal(ols->signals, "source_rename", &data);
    signal_handler_signal(source->context.signals, "rename", &data);
    calldata_free(&data);
    bfree(prev_name);
  }
}

enum ols_source_type ols_source_get_type(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_get_type")
             ? source->info.type
             : OLS_SOURCE_TYPE_INPUT;
}

const char *ols_source_get_id(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_get_id") ? source->info.id : NULL;
}

signal_handler_t *ols_source_get_signal_handler(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_get_signal_handler")
             ? source->context.signals
             : NULL;
}

proc_handler_t *ols_source_get_proc_handler(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_get_proc_handler")
             ? source->context.procs
             : NULL;
}

void ols_source_save(ols_source_t *source) {
  if (!data_valid(source, "ols_source_save"))
    return;

  ols_source_dosignal(source, "source_save", "save");

  if (source->info.save)
    source->info.save(source->context.data, source->context.settings);
}

void ols_source_load(ols_source_t *source) {
  if (!data_valid(source, "ols_source_load"))
    return;
  if (source->info.load)
    source->info.load(source->context.data, source->context.settings);

  ols_source_dosignal(source, "source_load", "load");
}

bool ols_source_active(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_active")
             ? source->activate_refs != 0
             : false;
}

bool ols_source_add_pad(ols_source_t *source, ols_pad_t *pad){

  if(! source || !pad){
    blog(LOG_ERROR,"source add pad with args NULL");
    return false;
  }

  return ols_context_add_pad(&source->context,pad);
}

static inline void signal_flags_updated(ols_source_t *source) {
  struct calldata data;
  uint8_t stack[128];

  calldata_init_fixed(&data, stack, sizeof(stack));
  calldata_set_ptr(&data, "source", source);
  calldata_set_int(&data, "flags", source->flags);

  signal_handler_signal(source->context.signals, "update_flags", &data);
}

void ols_source_set_flags(ols_source_t *source, uint32_t flags) {
  if (!ols_source_valid(source, "ols_source_set_flags"))
    return;

  if (flags != source->flags) {
    source->flags = flags;
    signal_flags_updated(source);
  }
}

void ols_source_set_default_flags(ols_source_t *source, uint32_t flags) {
  if (!ols_source_valid(source, "ols_source_set_default_flags"))
    return;

  source->default_flags = flags;
}

uint32_t ols_source_get_flags(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_get_flags") ? source->flags : 0;
}

ols_data_t *ols_source_get_private_settings(ols_source_t *source) {
  if (!ols_ptr_valid(source, "ols_source_get_private_settings"))
    return NULL;

  ols_data_addref(source->private_settings);
  return source->private_settings;
}

uint32_t ols_source_get_last_ols_version(const ols_source_t *source) {
  return ols_source_valid(source, "ols_source_get_last_ols_version")
             ? source->last_ols_ver
             : 0;
}

enum ols_icon_type ols_source_get_icon_type(const char *id) {
  const struct ols_source_info *info = get_source_info(id);
  return (info) ? info->icon_type : OLS_ICON_TYPE_UNKNOWN;
}
