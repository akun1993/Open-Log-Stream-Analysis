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

#include "callback/calldata.h"

#include "ols-internal.h"
#include "ols.h"

struct ols_core *ols = NULL;

static THREAD_LOCAL bool is_ui_thread = false;

extern void add_default_module_paths(void);
extern char *find_libols_data_file(const char *file);

static bool ols_init_data(void)
{
  struct ols_core_data *data = &ols->data;

  assert(data != NULL);

  if (pthread_mutex_init_recursive(&data->sources_mutex) != 0)
    goto fail;

  if (pthread_mutex_init_recursive(&data->outputs_mutex) != 0)
    goto fail;

  data->sources = NULL;
  data->public_sources = NULL;
  data->private_data = ols_data_create();
  data->valid = true;

fail:
  return data->valid;
}

#define FREE_OLS_HASH_TABLE(handle, table, type)                    \
  do                                                                \
  {                                                                 \
    struct ols_context_data *ctx, *tmp;                             \
    int unfreed = 0;                                                \
    HASH_ITER(handle, *(struct ols_context_data **)table, ctx, tmp) \
    {                                                               \
      ols_##type##_destroy((ols_##type##_t *)ctx);                  \
      unfreed++;                                                    \
    }                                                               \
    if (unfreed)                                                    \
      blog(LOG_INFO, "\t%d " #type "(s) were remaining", unfreed);  \
  } while (false)

#define FREE_OLS_LINKED_LIST(type)                                 \
  do                                                               \
  {                                                                \
    int unfreed = 0;                                               \
    while (data->first_##type)                                     \
    {                                                              \
      ols_##type##_destroy(data->first_##type);                    \
      unfreed++;                                                   \
    }                                                              \
    if (unfreed)                                                   \
      blog(LOG_INFO, "\t%d " #type "(s) were remaining", unfreed); \
  } while (false)

static void ols_free_data(void)
{
  struct ols_core_data *data = &ols->data;

  data->valid = false;

  blog(LOG_INFO, "Freeing OLS context data");

  FREE_OLS_LINKED_LIST(output);

  FREE_OLS_HASH_TABLE(hh, &data->public_sources, source);
  FREE_OLS_HASH_TABLE(hh_uuid, &data->sources, source);

  os_task_queue_wait(ols->destruction_task_thread);

  pthread_mutex_destroy(&data->sources_mutex);

  pthread_mutex_destroy(&data->outputs_mutex);

  da_free(data->tick_callbacks);
  ols_data_release(data->private_data);

  da_free(data->sources_to_tick);
}

static const char *ols_signals[] = {
    "void source_create(ptr source)",
    "void source_destroy(ptr source)",
    "void source_remove(ptr source)",
    "void source_update(ptr source)",
    "void source_save(ptr source)",
    "void source_load(ptr source)",
    "void source_rename(ptr source, string new_name, string prev_name)",
    "void channel_change(int channel, in out ptr source, ptr prev_source)",
    "void hotkey_layout_change()",
    "void hotkey_register(ptr hotkey)",
    "void hotkey_unregister(ptr hotkey)",
    "void hotkey_bindings_changed(ptr hotkey)",

    NULL,
};

static inline bool ols_init_handlers(void)
{
  ols->signals = signal_handler_create();
  if (!ols->signals)
    return false;

  ols->procs = proc_handler_create();
  if (!ols->procs)
    return false;

  return signal_handler_add_array(ols->signals, ols_signals);
}

static pthread_once_t ols_pthread_once_init_token = PTHREAD_ONCE_INIT;

extern void log_system_info(void);

static bool ols_init(const char *locale, const char *module_config_path,
                     profiler_name_store_t *store)
{
  ols = bzalloc(sizeof(struct ols_core));

  ols->name_store_owned = !store;
  ols->name_store = store ? store : profiler_name_store_create();
  if (!ols->name_store)
  {
    blog(LOG_ERROR, "Couldn't create profiler name store");
    return false;
  }

  log_system_info();

  if (!ols_init_data())
    return false;
  if (!ols_init_handlers())
    return false;

  ols->destruction_task_thread = os_task_queue_create();
  if (!ols->destruction_task_thread)
    return false;

  if (module_config_path)
    ols->module_config_path = bstrdup(module_config_path);
  ols->locale = bstrdup(locale);

  add_default_module_paths();
  return true;
}

#ifdef _WIN32
extern bool initialize_com(void);
extern void uninitialize_com(void);
static bool com_initialized = false;
#endif

/* Separate from actual context initialization
 * since this can be set before startup and persist
 * after shutdown. */
static DARRAY(struct dstr) core_module_paths = {0};

char *ols_find_data_file(const char *file)
{
  struct dstr path = {0};

  char *result = find_libols_data_file(file);
  if (result)
    return result;

  for (size_t i = 0; i < core_module_paths.num; ++i)
  {
    if (check_path(file, core_module_paths.array[i].array, &path))
      return path.array;
  }

  blog(LOG_ERROR, "Failed to find file '%s' in libols data directory", file);

  dstr_free(&path);
  return NULL;
}

void ols_add_data_path(const char *path)
{
  struct dstr *new_path = da_push_back_new(core_module_paths);
  dstr_init_copy(new_path, path);
}

bool ols_remove_data_path(const char *path)
{
  for (size_t i = 0; i < core_module_paths.num; ++i)
  {
    int result = dstr_cmp(&core_module_paths.array[i], path);

    if (result == 0)
    {
      dstr_free(&core_module_paths.array[i]);
      da_erase(core_module_paths, i);
      return true;
    }
  }

  return false;
}

static const char *ols_startup_name = "ols_startup";
bool ols_startup(const char *locale, const char *module_config_path,
                 profiler_name_store_t *store)
{
  bool success;

  profile_start(ols_startup_name);

  if (ols)
  {
    blog(LOG_WARNING, "Tried to call ols_startup more than once");
    return false;
  }

#ifdef _WIN32
  com_initialized = initialize_com();
#endif

  success = ols_init(locale, module_config_path, store);
  profile_end(ols_startup_name);
  if (!success)
    ols_shutdown();

  return success;
}

static struct ols_cmdline_args cmdline_args = {0, NULL};
void ols_set_cmdline_args(int argc, const char *const *argv)
{
  char *data;
  size_t len;
  int i;

  /* Once argc is set (non-zero) we shouldn't call again */
  if (cmdline_args.argc)
    return;

  cmdline_args.argc = argc;

  /* Safely copy over argv */
  len = 0;
  for (i = 0; i < argc; i++)
    len += strlen(argv[i]) + 1;

  cmdline_args.argv = bmalloc(sizeof(char *) * (argc + 1) + len);
  data = (char *)cmdline_args.argv + sizeof(char *) * (argc + 1);

  for (i = 0; i < argc; i++)
  {
    cmdline_args.argv[i] = data;
    len = strlen(argv[i]) + 1;
    memcpy(data, argv[i], len);
    data += len;
  }

  cmdline_args.argv[argc] = NULL;
}

struct ols_cmdline_args ols_get_cmdline_args(void)
{
  return cmdline_args;
}

void ols_shutdown(void)
{
  struct ols_module *module;

  ols_wait_for_destroy_queue();

  for (size_t i = 0; i < ols->source_types.num; i++)
  {
    struct ols_source_info *item = &ols->source_types.array[i];
    if (item->type_data && item->free_type_data)
      item->free_type_data(item->type_data);
    if (item->id)
      bfree((void *)item->id);
  }
  da_free(ols->source_types);

#define FREE_REGISTERED_TYPES(structure, list)     \
  do                                               \
  {                                                \
    for (size_t i = 0; i < list.num; i++)          \
    {                                              \
      struct structure *item = &list.array[i];     \
      if (item->type_data && item->free_type_data) \
        item->free_type_data(item->type_data);     \
    }                                              \
    da_free(list);                                 \
  } while (false)

  FREE_REGISTERED_TYPES(ols_output_info, ols->output_types);

#undef FREE_REGISTERED_TYPES

  // da_free(ols->input_types);

  module = ols->first_module;
  while (module)
  {
    struct ols_module *next = module->next;
    free_module(module);
    module = next;
  }
  ols->first_module = NULL;

  ols_free_data();
  os_task_queue_destroy(ols->destruction_task_thread);
  proc_handler_destroy(ols->procs);
  signal_handler_destroy(ols->signals);
  ols->procs = NULL;
  ols->signals = NULL;

  for (size_t i = 0; i < ols->module_paths.num; i++)
    free_module_path(ols->module_paths.array + i);
  da_free(ols->module_paths);

  for (size_t i = 0; i < ols->safe_modules.num; i++)
    bfree(ols->safe_modules.array[i]);
  da_free(ols->safe_modules);

  if (ols->name_store_owned)
    profiler_name_store_free(ols->name_store);

  bfree(ols->module_config_path);
  bfree(ols->locale);
  bfree(ols);
  ols = NULL;
  bfree(cmdline_args.argv);

#ifdef _WIN32
  if (com_initialized)
    uninitialize_com();
#endif
}

bool ols_initialized(void) { return ols != NULL; }

uint32_t ols_get_version(void) { return LIBOLS_API_VER; }

const char *ols_get_version_string(void) { return OLS_VERSION; }

void ols_set_locale(const char *locale)
{
  struct ols_module *module;

  if (ols->locale)
    bfree(ols->locale);
  ols->locale = bstrdup(locale);

  module = ols->first_module;
  while (module)
  {
    if (module->set_locale)
      module->set_locale(locale);

    module = module->next;
  }
}

const char *ols_get_locale(void) { return ols->locale; }

bool ols_enum_source_types(size_t idx, const char **id)
{
  if (idx >= ols->source_types.num)
    return false;
  *id = ols->source_types.array[idx].id;
  return true;
}

bool ols_enum_output_types(size_t idx, const char **id)
{
  if (idx >= ols->output_types.num)
    return false;
  *id = ols->output_types.array[idx].id;
  return true;
}

void ols_enum_sources(bool (*enum_proc)(void *, ols_source_t *), void *param)
{
  ols_source_t *source;

  pthread_mutex_lock(&ols->data.sources_mutex);
  source = ols->data.public_sources;

  while (source)
  {
    ols_source_t *s = ols_source_get_ref(source);
    if (s)
    {
      if (s->info.type == OLS_SOURCE_TYPE_INPUT && !enum_proc(param, s))
      {
        ols_source_release(s);
        break;
      }
      ols_source_release(s);
    }

    source = (ols_source_t *)source->context.hh.next;
  }

  pthread_mutex_unlock(&ols->data.sources_mutex);
}

static inline void ols_enum(void *pstart, pthread_mutex_t *mutex, void *proc,
                            void *param)
{
  struct ols_context_data **start = pstart, *context;
  bool (*enum_proc)(void *, void *) = proc;

  assert(start);
  assert(mutex);
  assert(enum_proc);

  pthread_mutex_lock(mutex);

  context = *start;
  if (context)
  {
    enum_proc(param, context);
  }

  pthread_mutex_unlock(mutex);
}

static inline void ols_enum_uuid(void *pstart, pthread_mutex_t *mutex,
                                 void *proc, void *param)
{
  struct ols_context_data **start = pstart, *context, *tmp;
  bool (*enum_proc)(void *, void *) = proc;

  assert(start);
  assert(mutex);
  assert(enum_proc);

  pthread_mutex_lock(mutex);

  HASH_ITER(hh_uuid, *start, context, tmp)
  {
    if (!enum_proc(param, context))
      break;
  }

  pthread_mutex_unlock(mutex);
}

void ols_enum_all_sources(bool (*enum_proc)(void *, ols_source_t *),
                          void *param)
{
  ols_enum_uuid(&ols->data.sources, &ols->data.sources_mutex, enum_proc, param);
}

void ols_enum_outputs(bool (*enum_proc)(void *, ols_output_t *), void *param)
{
  ols_enum(&ols->data.first_output, &ols->data.outputs_mutex, enum_proc, param);
}

static void *get_context_by_uuid(void *ptable, const char *uuid,
                                 pthread_mutex_t *mutex,
                                 void *(*addref)(void *))
{
  struct ols_context_data **ht = ptable;
  struct ols_context_data *context;

  pthread_mutex_lock(mutex);

  HASH_FIND_UUID(*ht, uuid, context);
  if (context)
    addref(context);

  pthread_mutex_unlock(mutex);
  return context;
}

static inline void *ols_source_addref_safe_(void *ref)
{
  return ols_source_get_ref(ref);
}

static inline void *ols_output_addref_safe_(void *ref)
{
  return ols_output_get_ref(ref);
}

static inline void *ols_id_(void *data) { return data; }

ols_source_t *ols_get_source_by_uuid(const char *uuid)
{
  return get_context_by_uuid(&ols->data.sources, uuid, &ols->data.sources_mutex,
                             ols_source_addref_safe_);
}

signal_handler_t *ols_get_signal_handler(void) { return ols->signals; }

proc_handler_t *ols_get_proc_handler(void) { return ols->procs; }

static ols_source_t *ols_load_source_type(ols_data_t *source_data,
                                          bool is_private)
{
  ols_source_t *source;
  const char *name = ols_data_get_string(source_data, "name");
  const char *uuid = ols_data_get_string(source_data, "uuid");
  const char *id = ols_data_get_string(source_data, "id");
  const char *v_id = ols_data_get_string(source_data, "versioned_id");
  ols_data_t *settings = ols_data_get_obj(source_data, "settings");

  uint32_t prev_ver;
  uint32_t flags;

  prev_ver = (uint32_t)ols_data_get_int(source_data, "prev_ver");

  if (!*v_id)
    v_id = id;

  source = ols_source_create_set_last_ver(v_id, name, uuid, settings, prev_ver, is_private);

  ols_data_set_default_int(source_data, "flags", source->default_flags);
  flags = (uint32_t)ols_data_get_int(source_data, "flags");
  ols_source_set_flags(source, flags);

  ols_data_release(source->private_settings);
  source->private_settings = ols_data_get_obj(source_data, "private_settings");
  if (!source->private_settings)
    source->private_settings = ols_data_create();

  ols_data_release(settings);

  return source;
}

ols_source_t *ols_load_source(ols_data_t *source_data)
{
  return ols_load_source_type(source_data, false);
}

ols_source_t *ols_load_private_source(ols_data_t *source_data)
{
  return ols_load_source_type(source_data, true);
}

void ols_load_sources(ols_data_array_t *array, ols_load_source_cb cb,
                      void *private_data)
{
  struct ols_core_data *data = &ols->data;
  DARRAY(ols_source_t *)
  sources;
  size_t count;
  size_t i;

  da_init(sources);

  count = ols_data_array_count(array);
  da_reserve(sources, count);

  pthread_mutex_lock(&data->sources_mutex);

  for (i = 0; i < count; i++)
  {
    ols_data_t *source_data = ols_data_array_item(array, i);
    ols_source_t *source = ols_load_source(source_data);

    da_push_back(sources, &source);

    ols_data_release(source_data);
  }

  /* tell sources that we want to load */
  for (i = 0; i < sources.num; i++)
  {
    ols_source_t *source = sources.array[i];
    ols_data_t *source_data = ols_data_array_item(array, i);
    if (source)
    {
      if (cb)
        cb(private_data, source);
    }
    ols_data_release(source_data);
  }

  for (i = 0; i < sources.num; i++)
    ols_source_release(sources.array[i]);

  pthread_mutex_unlock(&data->sources_mutex);

  da_free(sources);
}

ols_data_t *ols_save_source(ols_source_t *source)
{
  ols_data_t *source_data = ols_data_create();
  ols_data_t *settings = ols_source_get_settings(source);

  uint32_t flags = ols_source_get_flags(source);
  const char *name = ols_source_get_name(source);
  const char *uuid = ols_source_get_uuid(source);
  const char *v_id = source->info.id;

  ols_source_save(source);

  ols_data_set_int(source_data, "prev_ver", LIBOLS_API_VER);

  ols_data_set_string(source_data, "name", name);
  ols_data_set_string(source_data, "uuid", uuid);
  ols_data_set_string(source_data, "versioned_id", v_id);
  ols_data_set_obj(source_data, "settings", settings);
  ols_data_set_int(source_data, "flags", flags);

  ols_data_set_obj(source_data, "private_settings", source->private_settings);

  ols_data_release(settings);

  return source_data;
}

ols_data_array_t *ols_save_sources_filtered(ols_save_source_filter_cb cb,
                                            void *data_)
{
  struct ols_core_data *data = &ols->data;
  ols_data_array_t *array;
  ols_source_t *source;

  array = ols_data_array_create();

  pthread_mutex_lock(&data->sources_mutex);

  source = data->public_sources;

  while (source)
  {
    if ((source->info.type != OLS_SOURCE_TYPE_FILTER) != 0 &&
        !source->removed && !source->temp_removed && cb(data_, source))
    {
      ols_data_t *source_data = ols_save_source(source);

      ols_data_array_push_back(array, source_data);
      ols_data_release(source_data);
    }

    source = (ols_source_t *)source->context.hh.next;
  }

  pthread_mutex_unlock(&data->sources_mutex);

  return array;
}

static bool save_source_filter(void *data, ols_source_t *source)
{
  UNUSED_PARAMETER(data);
  UNUSED_PARAMETER(source);
  return true;
}

ols_data_array_t *ols_save_sources(void)
{
  return ols_save_sources_filtered(save_source_filter, NULL);
}

void ols_reset_source_uuids()
{
  pthread_mutex_lock(&ols->data.sources_mutex);

  /* Move all sources to a new hash table */
  struct ols_context_data *ht = (struct ols_context_data *)ols->data.sources;
  struct ols_context_data *new_ht = NULL;

  struct ols_context_data *ctx, *tmp;
  HASH_ITER(hh_uuid, ht, ctx, tmp)
  {
    HASH_DELETE(hh_uuid, ht, ctx);

    bfree((void *)ctx->uuid);
    ctx->uuid = os_generate_uuid();

    HASH_ADD_UUID(new_ht, uuid, ctx);
  }

  /* The old table will be automatically freed once the last element has
   * been removed, so we can simply overwrite the pointer. */
  ols->data.sources = (struct ols_source *)new_ht;

  pthread_mutex_unlock(&ols->data.sources_mutex);
}

/* ensures that names are never blank */
static inline char *dup_name(const char *name, bool private)
{
  if (private && !name)
    return NULL;

  if (!name || !*name)
  {
    struct dstr unnamed = {0};
    dstr_printf(&unnamed, "__unnamed%04lld", ols->data.unnamed_index++);

    return unnamed.array;
  }
  else
  {
    return bstrdup(name);
  }
}

static inline bool
ols_context_data_init_wrap(struct ols_context_data *context,
                           enum ols_obj_type type, ols_data_t *settings,
                           const char *name, const char *uuid, bool private)
{
  assert(context);
  memset(context, 0, sizeof(*context));
  context->private = private;
  context->type = type;

  context->signals = signal_handler_create();
  if (!context->signals)
    return false;

  context->procs = proc_handler_create();
  if (!context->procs)
    return false;

  if (uuid && strlen(uuid) == UUID_STR_LENGTH)
    context->uuid = bstrdup(uuid);
  /* Only automatically generate UUIDs for sources */
  else if (type == OLS_OBJ_TYPE_SOURCE)
    context->uuid = os_generate_uuid();

  context->name = dup_name(name, private);
  context->settings = ols_data_newref(settings);
  return true;
}

bool ols_context_data_init(struct ols_context_data *context,
                           enum ols_obj_type type, ols_data_t *settings,
                           const char *name, const char *uuid, bool private)
{
  if (ols_context_data_init_wrap(context, type, settings, name, uuid, private))
  {
    return true;
  }
  else
  {
    ols_context_data_free(context);
    return false;
  }
}

void ols_context_data_free(struct ols_context_data *context)
{

  signal_handler_destroy(context->signals);
  proc_handler_destroy(context->procs);
  ols_data_release(context->settings);
  ols_context_data_remove(context);

  bfree(context->name);
  bfree((void *)context->uuid);

  memset(context, 0, sizeof(*context));
}

void ols_context_init_control(struct ols_context_data *context, void *object,
                              ols_destroy_cb destroy)
{
  context->control = bzalloc(sizeof(ols_weak_object_t));
  context->control->object = object;
  context->destroy = destroy;
}

static inline char *ols_context_deduplicate_name(void *phash,
                                                 const char *name)
{
  struct ols_context_data *head = phash;
  struct ols_context_data *item = NULL;

  HASH_FIND_STR(head, name, item);
  if (!item)
    return NULL;

  struct dstr new_name = {0};
  int suffix = 2;

  while (item)
  {
    dstr_printf(&new_name, "%s %d", name, suffix++);
    HASH_FIND_STR(head, new_name.array, item);
  }

  return new_name.array;
}

void ols_context_data_insert_uuid(struct ols_context_data *context,
                                  pthread_mutex_t *mutex, void *pfirst_uuid)
{
  struct ols_context_data **first_uuid = pfirst_uuid;
  struct ols_context_data *item = NULL;

  assert(context);
  assert(mutex);
  assert(first_uuid);

  context->mutex = mutex;

  pthread_mutex_lock(mutex);

  /* Ensure UUID is not a duplicate.
   * This should only ever happen if a scene collection file has been
   * manually edited and an entry has been duplicated without removing
   * or regenerating the UUID. */
  HASH_FIND_UUID(*first_uuid, context->uuid, item);
  if (item)
  {
    blog(LOG_WARNING, "Attempted to insert context with duplicate UUID \"%s\"!",
         context->uuid);
    /* It is practically impossible for the new UUID to be a
     * duplicate, so don't bother checking again. */
    bfree((void *)context->uuid);
    context->uuid = os_generate_uuid();
  }

  HASH_ADD_UUID(*first_uuid, uuid, context);
  pthread_mutex_unlock(mutex);
}

void ols_context_data_remove(struct ols_context_data *context)
{
  // if (context && context->prev_next) {
  // 	pthread_mutex_lock(context->mutex);
  // 	*context->prev_next = context->next;
  // 	if (context->next)
  // 		context->next->prev_next = context->prev_next;
  // 	context->prev_next = NULL;
  // 	pthread_mutex_unlock(context->mutex);
  // }
  UNUSED_PARAMETER(context);
}

void ols_context_data_remove_name(struct ols_context_data *context,
                                  void *phead)
{
  struct ols_context_data **head = phead;

  assert(head);

  if (!context)
    return;

  pthread_mutex_lock(context->mutex);
  HASH_DELETE(hh, *head, context);
  pthread_mutex_unlock(context->mutex);
}

void ols_context_data_remove_uuid(struct ols_context_data *context,
                                  void *puuid_head)
{
  struct ols_context_data **uuid_head = puuid_head;

  assert(uuid_head);

  if (!context || !context->uuid || !uuid_head)
    return;

  pthread_mutex_lock(context->mutex);
  HASH_DELETE(hh_uuid, *uuid_head, context);
  pthread_mutex_unlock(context->mutex);
}

void ols_context_wait(struct ols_context_data *context)
{
  pthread_mutex_lock(context->mutex);
  pthread_mutex_unlock(context->mutex);
}

void ols_context_data_setname_ht(struct ols_context_data *context,
                                 const char *name, void *phead)
{
  struct ols_context_data **head = phead;
  char *new_name;

  pthread_mutex_lock(context->mutex);

  HASH_DEL(*head, context);

  /* Ensure new name is not a duplicate. */
  new_name = ols_context_deduplicate_name(*head, name);
  if (new_name)
  {
    blog(LOG_WARNING,
         "Attempted to rename context to duplicate name \"%s\"!"
         " New name has been changed to \"%s\"",
         context->name, new_name);
    context->name = new_name;
  }
  else
  {
    context->name = dup_name(name, context->private);
  }

  HASH_ADD_STR(*head, name, context);

  pthread_mutex_unlock(context->mutex);
}

profiler_name_store_t *ols_get_profiler_name_store(void)
{
  return ols->name_store;
}

enum ols_obj_type ols_obj_get_type(void *obj)
{
  struct ols_context_data *context = obj;
  return context ? context->type : OLS_OBJ_TYPE_INVALID;
}

const char *ols_obj_get_id(void *obj)
{
  struct ols_context_data *context = obj;
  if (!context)
    return NULL;

  switch (context->type)
  {
  case OLS_OBJ_TYPE_SOURCE:
    return ((ols_source_t *)obj)->info.id;
  case OLS_OBJ_TYPE_OUTPUT:
    return ((ols_output_t *)obj)->info.id;
  default:;
  }

  return NULL;
}

bool ols_obj_invalid(void *obj)
{
  struct ols_context_data *context = obj;
  if (!context)
    return true;

  return !context->data;
}

void *ols_obj_get_data(void *obj)
{
  struct ols_context_data *context = obj;
  if (!context)
    return NULL;

  return context->data;
}

bool ols_obj_is_private(void *obj)
{
  struct ols_context_data *context = obj;
  if (!context)
    return false;

  return context->private;
}

void ols_apply_private_data(ols_data_t *settings)
{
  if (!settings)
    return;

  ols_data_apply(ols->data.private_data, settings);
}

void ols_set_private_data(ols_data_t *settings)
{
  ols_data_clear(ols->data.private_data);
  if (settings)
    ols_data_apply(ols->data.private_data, settings);
}

ols_data_t *ols_get_private_data(void)
{
  ols_data_t *private_data = ols->data.private_data;
  ols_data_addref(private_data);
  return private_data;
}

/* ------------------------------------------------------------------------- */
/* task stuff                                                                */

struct task_wait_info
{
  ols_task_t task;
  void *param;
  os_event_t *event;
};

static void task_wait_callback(void *param)
{
  struct task_wait_info *info = param;
  if (info->task)
    info->task(info->param);
  os_event_signal(info->event);
}

bool ols_in_task_thread(enum ols_task_type type)
{

  if (type == OLS_TASK_UI)
    return is_ui_thread;
  else if (type == OLS_TASK_DESTROY)
    return os_task_queue_inside(ols->destruction_task_thread);

  assert(false);
  return false;
}

void ols_queue_task(enum ols_task_type type, ols_task_t task, void *param,
                    bool wait)
{
  if (type == OLS_TASK_UI)
  {
    if (ols->ui_task_handler)
    {
      ols->ui_task_handler(task, param, wait);
    }
    else
    {
      blog(LOG_ERROR, "UI task could not be queued, "
                      "there's no UI task handler!");
    }
  }
  else
  {
    if (ols_in_task_thread(type))
    {
      task(param);
    }
    else if (wait)
    {
      struct task_wait_info info = {
          .task = task,
          .param = param,
      };

      os_event_init(&info.event, OS_EVENT_TYPE_MANUAL);
      ols_queue_task(type, task_wait_callback, &info, false);
      os_event_wait(info.event);
      os_event_destroy(info.event);
    }
    else if (type == OLS_TASK_DESTROY)
    {
      os_task_t os_task = (os_task_t)task;
      os_task_queue_queue_task(ols->destruction_task_thread, os_task, param);
    }
  }
}

bool ols_wait_for_destroy_queue(void)
{
  /* wait for destroy task queue */
  return os_task_queue_wait(ols->destruction_task_thread);
}

static void set_ui_thread(void *unused)
{
  is_ui_thread = true;
  UNUSED_PARAMETER(unused);
}

void ols_set_ui_task_handler(ols_task_handler_t handler)
{
  ols->ui_task_handler = handler;
  ols_queue_task(OLS_TASK_UI, set_ui_thread, NULL, false);
}

ols_object_t *ols_object_get_ref(ols_object_t *object)
{
  if (!object)
    return NULL;

  return ols_weak_object_get_object(object->control);
}

void ols_object_release(ols_object_t *object)
{
  if (!ols)
  {
    blog(LOG_WARNING, "Tried to release an object when the OLS "
                      "core is shut down!");
    return;
  }

  if (!object)
    return;

  ols_weak_object_t *control = object->control;
  if (ols_ref_release(&control->ref))
  {
    object->destroy(object);
    ols_weak_object_release(control);
  }
}

void ols_weak_object_addref(ols_weak_object_t *weak)
{
  if (!weak)
    return;

  ols_weak_ref_addref(&weak->ref);
}

void ols_weak_object_release(ols_weak_object_t *weak)
{
  if (!weak)
    return;

  if (ols_weak_ref_release(&weak->ref))
    bfree(weak);
}

ols_weak_object_t *ols_object_get_weak_object(ols_object_t *object)
{
  if (!object)
    return NULL;

  ols_weak_object_t *weak = object->control;
  ols_weak_object_addref(weak);
  return weak;
}

ols_object_t *ols_weak_object_get_object(ols_weak_object_t *weak)
{
  if (!weak)
    return NULL;

  if (ols_weak_ref_get_ref(&weak->ref))
    return weak->object;

  return NULL;
}

bool ols_weak_object_expired(ols_weak_object_t *weak)
{
  return weak ? ols_weak_ref_expired(&weak->ref) : true;
}

bool ols_weak_object_references_object(ols_weak_object_t *weak,
                                       ols_object_t *object)
{
  return weak && object && weak->object == object;
}

bool ols_is_output_protocol_registered(const char *protocol)
{
  for (size_t i = 0; i < ols->data.protocols.num; i++)
  {
    if (strcmp(protocol, ols->data.protocols.array[i]) == 0)
      return true;
  }

  return false;
}

bool ols_enum_output_protocols(size_t idx, char **protocol)
{
  if (idx >= ols->data.protocols.num)
    return false;

  *protocol = ols->data.protocols.array[idx];
  return true;
}
