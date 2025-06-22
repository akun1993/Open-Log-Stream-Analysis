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

#include "ols-internal.h"
#include "ols.h"
#include "util/platform.h"
#include "util/util_uint64.h"
#include <inttypes.h>

#define get_weak(output) ((ols_weak_output_t *)output->context.control)

#define RECONNECT_RETRY_MAX_MSEC (15 * 60 * 1000)
#define RECONNECT_RETRY_BASE_EXP 1.5f

static inline bool reconnecting(const struct ols_output *output)
{
  return os_atomic_load_bool(&output->reconnecting);
}

const struct ols_output_info *find_output(const char *id)
{
  size_t i;
  for (i = 0; i < ols->output_types.num; i++)
    if (strcmp(ols->output_types.array[i].id, id) == 0)
      return ols->output_types.array + i;

  return NULL;
}

const char *ols_output_get_display_name(const char *id)
{
  const struct ols_output_info *info = find_output(id);
  return (info != NULL) ? info->get_name(info->type_data) : NULL;
}

static const char *output_signals[] = {
    "void start(ptr output)",
    "void stop(ptr output, int code)",
    "void starting(ptr output)",
    "void stopping(ptr output)",
    "void activate(ptr output)",
    "void deactivate(ptr output)",
    "void reconnect(ptr output)",
    "void reconnect_success(ptr output)",
    NULL,
};

static bool init_output_handlers(struct ols_output *output, const char *name,
                                 ols_data_t *settings)
{
  if (!ols_context_data_init(&output->context, OLS_OBJ_TYPE_OUTPUT, settings,
                             name, NULL))
    return false;

  signal_handler_add_array(output->context.signals, output_signals);
  return true;
}

ols_output_t *ols_output_create(const char *id, const char *name,
                                ols_data_t *settings)
{
  const struct ols_output_info *info = find_output(id);
  struct ols_output *output;
  int ret;

  output = bzalloc(sizeof(struct ols_output));

  if (!init_output_handlers(output, name, settings))
    goto fail;

  if (!info)
  {
    blog(LOG_ERROR, "Output ID '%s' not found", id);

    output->info.id = bstrdup(id);
    output->owns_info_id = true;
  }
  else
  {
    output->info = *info;
  }

  if (output->info.get_defaults)
    output->info.get_defaults(output->context.settings);

  ret = os_event_init(&output->reconnect_stop_event, OS_EVENT_TYPE_MANUAL);
  if (ret < 0)
    goto fail;

  output->reconnect_retry_sec = 2;
  output->reconnect_retry_max = 20;
  output->reconnect_retry_exp = RECONNECT_RETRY_BASE_EXP + (rand() * 0.05f);
  output->valid = true;

  ols_context_init_control(&output->context, output,
                           (ols_destroy_cb)ols_output_destroy);
  // ols_context_data_insert(&output->context, &ols->data.outputs_mutex,
  // 			&ols->data.first_output);

  if (info)
    output->context.data = info->create(output->context.settings, output);
  if (!output->context.data)
    blog(LOG_ERROR, "Failed to create output '%s'!", name);

  blog(LOG_DEBUG, "output '%s' (%s) created", name, id);
  return output;

fail:
  ols_output_destroy(output);
  return NULL;
}

void ols_output_destroy(ols_output_t *output)
{
  if (output)
  {
    ols_context_data_remove(&output->context);

    blog(LOG_DEBUG, "output '%s' destroyed", output->context.name);

    if (output->valid)
      ols_output_actual_stop(output, true, 0);

    if (output->context.data)
      output->info.destroy(output->context.data);

    os_event_destroy(output->reconnect_stop_event);
    ols_context_data_free(&output->context);

    if (output->owns_info_id)
      bfree((void *)output->info.id);
    if (output->last_error_message)
      bfree(output->last_error_message);
    bfree(output);
  }
}

const char *ols_output_get_name(const ols_output_t *output)
{
  return ols_output_valid(output, "ols_output_get_name") ? output->context.name
                                                         : NULL;
}

bool ols_output_actual_start(ols_output_t *output)
{
  bool success = false;

  UNUSED_PARAMETER(output);

  return success;
}

bool ols_output_start(ols_output_t *output)
{
  if (!ols_output_valid(output, "ols_output_start"))
    return false;
  if (!output->context.data)
    return false;

  if (ols_output_actual_start(output))
  {
    do_output_signal(output, "starting");
    return true;
  }

  return false;
}

static inline bool data_active(struct ols_output *output)
{
  return os_atomic_load_bool(&output->data_active);
}

static inline void signal_stop(struct ols_output *output);

void ols_output_actual_stop(ols_output_t *output, bool force, uint64_t ts)
{

  UNUSED_PARAMETER(output);
  UNUSED_PARAMETER(force);
  UNUSED_PARAMETER(ts);
}

void ols_output_stop(ols_output_t *output) { UNUSED_PARAMETER(output); }

uint32_t ols_output_get_flags(const ols_output_t *output)
{
  return ols_output_valid(output, "ols_output_get_flags") ? output->info.flags
                                                          : 0;
}

uint32_t ols_get_output_flags(const char *id)
{
  const struct ols_output_info *info = find_output(id);
  return info ? info->flags : 0;
}

static inline ols_data_t *get_defaults(const struct ols_output_info *info)
{
  ols_data_t *settings = ols_data_create();
  if (info->get_defaults)
    info->get_defaults(settings);
  return settings;
}

ols_data_t *ols_output_defaults(const char *id)
{
  const struct ols_output_info *info = find_output(id);
  return (info) ? get_defaults(info) : NULL;
}

ols_properties_t *ols_get_output_properties(const char *id)
{
  const struct ols_output_info *info = find_output(id);
  if (info && info->get_properties)
  {
    ols_data_t *defaults = get_defaults(info);
    ols_properties_t *properties;

    properties = info->get_properties(NULL);
    ols_properties_apply_settings(properties, defaults);
    ols_data_release(defaults);
    return properties;
  }
  return NULL;
}

ols_properties_t *ols_output_properties(const ols_output_t *output)
{
  if (!ols_output_valid(output, "ols_output_properties"))
    return NULL;

  if (output && output->info.get_properties)
  {
    ols_properties_t *props;
    props = output->info.get_properties(output->context.data);
    ols_properties_apply_settings(props, output->context.settings);
    return props;
  }

  return NULL;
}

void ols_output_update(ols_output_t *output, ols_data_t *settings)
{
  if (!ols_output_valid(output, "ols_output_update"))
    return;

  ols_data_apply(output->context.settings, settings);

  if (output->info.update)
    output->info.update(output->context.data, output->context.settings);
}

ols_data_t *ols_output_get_settings(const ols_output_t *output)
{
  if (!ols_output_valid(output, "ols_output_get_settings"))
    return NULL;

  ols_data_addref(output->context.settings);
  return output->context.settings;
}

signal_handler_t *ols_output_get_signal_handler(const ols_output_t *output)
{
  return ols_output_valid(output, "ols_output_get_signal_handler")
             ? output->context.signals
             : NULL;
}

proc_handler_t *ols_output_get_proc_handler(const ols_output_t *output)
{
  return ols_output_valid(output, "ols_output_get_proc_handler")
             ? output->context.procs
             : NULL;
}

void ols_output_set_reconnect_settings(ols_output_t *output, int retry_count,
                                       int retry_sec)
{
  if (!ols_output_valid(output, "ols_output_set_reconnect_settings"))
    return;

  output->reconnect_retry_max = retry_count;
  output->reconnect_retry_sec = retry_sec;
}

static inline void signal_start(struct ols_output *output)
{
  do_output_signal(output, "start");
}

static inline void signal_reconnect(struct ols_output *output)
{
  struct calldata params;
  uint8_t stack[128];

  calldata_init_fixed(&params, stack, sizeof(stack));
  calldata_set_int(&params, "timeout_sec",
                   output->reconnect_retry_cur_msec / 1000);
  calldata_set_ptr(&params, "output", output);
  signal_handler_signal(output->context.signals, "reconnect", &params);
}

static inline void signal_reconnect_success(struct ols_output *output)
{
  do_output_signal(output, "reconnect_success");
}

static inline void signal_stop(struct ols_output *output)
{
  struct calldata params;

  calldata_init(&params);
  calldata_set_string(&params, "last_error", ols_output_get_last_error(output));
  calldata_set_int(&params, "code", output->stop_code);
  calldata_set_ptr(&params, "output", output);

  signal_handler_signal(output->context.signals, "stop", &params);

  calldata_free(&params);
}

bool ols_output_begin_data_capture(ols_output_t *output, uint32_t flags)
{
  UNUSED_PARAMETER(flags);

  if (!ols_output_valid(output, "ols_output_begin_data_capture"))
    return false;

  output->total_frames = 0;

  os_atomic_set_bool(&output->data_active, true);

  do_output_signal(output, "activate");
  // os_atomic_set_bool(&output->active, true);

  if (reconnecting(output))
  {
    signal_reconnect_success(output);
    os_atomic_set_bool(&output->reconnecting, false);
  }
  else
  {
    signal_start(output);
  }

  return true;
}

static inline bool can_reconnect(const ols_output_t *output, int code)
{
  bool reconnect_active = output->reconnect_retry_max != 0;

  return (reconnecting(output) && code != OLS_OUTPUT_SUCCESS) ||
         (reconnect_active && code == OLS_OUTPUT_DISCONNECTED);
}

void ols_output_signal_stop(ols_output_t *output, int code)
{
  if (!ols_output_valid(output, "ols_output_signal_stop"))
    return;

  output->stop_code = code;
}

void ols_output_addref(ols_output_t *output)
{
  if (!output)
    return;

  ols_ref_addref(&output->context.control->ref);
}

void ols_output_release(ols_output_t *output)
{
  if (!output)
    return;

  ols_weak_output_t *control = get_weak(output);
  if (ols_ref_release(&control->ref))
  {
    // The order of operations is important here since
    // get_context_by_name in ols.c relies on weak refs
    // being alive while the context is listed
    ols_output_destroy(output);
    ols_weak_output_release(control);
  }
}

void ols_weak_output_addref(ols_weak_output_t *weak)
{
  if (!weak)
    return;

  ols_weak_ref_addref(&weak->ref);
}

void ols_weak_output_release(ols_weak_output_t *weak)
{
  if (!weak)
    return;

  if (ols_weak_ref_release(&weak->ref))
    bfree(weak);
}

ols_output_t *ols_output_get_ref(ols_output_t *output)
{
  if (!output)
    return NULL;

  return ols_weak_output_get_output(get_weak(output));
}

ols_weak_output_t *ols_output_get_weak_output(ols_output_t *output)
{
  if (!output)
    return NULL;

  ols_weak_output_t *weak = get_weak(output);
  ols_weak_output_addref(weak);
  return weak;
}

ols_output_t *ols_weak_output_get_output(ols_weak_output_t *weak)
{
  if (!weak)
    return NULL;

  if (ols_weak_ref_get_ref(&weak->ref))
    return weak->output;

  return NULL;
}

bool ols_weak_output_references_output(ols_weak_output_t *weak,
                                       ols_output_t *output)
{
  return weak && output && weak->output == output;
}

void *ols_output_get_type_data(ols_output_t *output)
{
  return ols_output_valid(output, "ols_output_get_type_data")
             ? output->info.type_data
             : NULL;
}

const char *ols_output_get_id(const ols_output_t *output)
{
  return ols_output_valid(output, "ols_output_get_id") ? output->info.id : NULL;
}

const char *ols_output_get_last_error(ols_output_t *output)
{
  if (!ols_output_valid(output, "ols_output_get_last_error"))
    return NULL;

  if (output->last_error_message)
  {
    return output->last_error_message;
  }
  else
  {
  }

  return NULL;
}

void ols_output_set_last_error(ols_output_t *output, const char *message)
{
  if (!ols_output_valid(output, "ols_output_set_last_error"))
    return;

  if (output->last_error_message)
    bfree(output->last_error_message);

  if (message)
    output->last_error_message = bstrdup(message);
  else
    output->last_error_message = NULL;
}

bool ols_output_reconnecting(const ols_output_t *output)
{
  if (!ols_output_valid(output, "ols_output_reconnecting"))
    return false;

  return reconnecting(output);
}
