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

#include <ols.h>
#include <util/deque.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/threading.h>

#include "ols-scripting-callback.h"
#include "ols-scripting-internal.h"

#if defined(LUAJIT_FOUND)
extern ols_script_t *ols_lua_script_create(const char *path,
                                           ols_data_t *settings);
extern bool ols_lua_script_load(ols_script_t *s);
extern void ols_lua_script_unload(ols_script_t *s);
extern void ols_lua_script_destroy(ols_script_t *s);
extern void ols_lua_load(void);
extern void ols_lua_unload(void);
extern void ols_lua_parse_data(ols_script_t *s, const char *data, int len);

#endif

#if defined(Python_FOUND)
extern ols_script_t *ols_python_script_create(const char *path,
                                              ols_data_t *settings);
extern bool ols_python_script_load(ols_script_t *s);
extern void ols_python_script_unload(ols_script_t *s);
extern void ols_python_script_destroy(ols_script_t *s);
extern void ols_python_load(void);
extern void ols_python_unload(void);

#endif

pthread_mutex_t detach_mutex;
struct script_callback *detached_callbacks;

static bool scripting_loaded = false;

static const char *supported_formats[] = {
#if defined(LUAJIT_FOUND)
    "lua",
#endif
#if defined(Python_FOUND)
    "py",
#endif
    NULL};

/* -------------------------------------------- */

static pthread_mutex_t defer_call_mutex;
static struct deque defer_call_queue;
static bool defer_call_exit = false;
static os_sem_t *defer_call_semaphore;
static pthread_t defer_call_thread;

struct defer_call {
  defer_call_cb call;
  void *cb;
};

static void *defer_thread(void *unused) {
  UNUSED_PARAMETER(unused);
  os_set_thread_name("scripting: defer");

  while (os_sem_wait(defer_call_semaphore) == 0) {
    struct defer_call info;

    pthread_mutex_lock(&defer_call_mutex);
    if (defer_call_exit) {
      pthread_mutex_unlock(&defer_call_mutex);
      return NULL;
    }

    deque_pop_front(&defer_call_queue, &info, sizeof(info));
    pthread_mutex_unlock(&defer_call_mutex);

    info.call(info.cb);
  }

  return NULL;
}

void defer_call_post(defer_call_cb call, void *cb) {
  struct defer_call info;
  info.call = call;
  info.cb = cb;

  pthread_mutex_lock(&defer_call_mutex);
  if (!defer_call_exit)
    deque_push_back(&defer_call_queue, &info, sizeof(info));
  pthread_mutex_unlock(&defer_call_mutex);

  os_sem_post(defer_call_semaphore);
}

/* -------------------------------------------- */

bool ols_scripting_load(void) {

  deque_init(&defer_call_queue);

  if (pthread_mutex_init(&detach_mutex, NULL) != 0) {
    return false;
  }
  if (pthread_mutex_init(&defer_call_mutex, NULL) != 0) {
    pthread_mutex_destroy(&detach_mutex);
    return false;
  }
  if (os_sem_init(&defer_call_semaphore, 0) != 0) {
    pthread_mutex_destroy(&defer_call_mutex);
    pthread_mutex_destroy(&detach_mutex);
    return false;
  }

  if (pthread_create(&defer_call_thread, NULL, defer_thread, NULL) != 0) {
    os_sem_destroy(defer_call_semaphore);
    pthread_mutex_destroy(&defer_call_mutex);
    pthread_mutex_destroy(&detach_mutex);
    return false;
  }

#if defined(LUAJIT_FOUND)
  ols_lua_load();
#endif

#if defined(Python_FOUND)
  ols_python_load();
#if !defined(_WIN32) &&                                                        \
    !defined(__APPLE__) /* Win32 and macOS need user-provided Python library   \
                           paths */
  ols_scripting_load_python(NULL);
#endif
#endif

  scripting_loaded = true;
  return true;
}

void ols_scripting_unload(void) {
  if (!scripting_loaded)
    return;

  /* ---------------------- */

#if defined(LUAJIT_FOUND)
  ols_lua_unload();
#endif

#if defined(Python_FOUND)
  ols_python_unload();
#endif
  /* ---------------------- */

  int total_detached = 0;

  pthread_mutex_lock(&detach_mutex);

  struct script_callback *cur = detached_callbacks;
  while (cur) {
    struct script_callback *next = cur->next;
    just_free_script_callback(cur);
    cur = next;

    ++total_detached;
  }

  pthread_mutex_unlock(&detach_mutex);
  pthread_mutex_destroy(&detach_mutex);

  blog(LOG_INFO, "[Scripting] Total detached callbacks: %d", total_detached);

  /* ---------------------- */

  pthread_mutex_lock(&defer_call_mutex);

  /* TODO */

  defer_call_exit = true;
  deque_free(&defer_call_queue);

  pthread_mutex_unlock(&defer_call_mutex);

  os_sem_post(defer_call_semaphore);
  pthread_join(defer_call_thread, NULL);

  pthread_mutex_destroy(&defer_call_mutex);
  os_sem_destroy(defer_call_semaphore);

  scripting_loaded = false;
}

const char **ols_scripting_supported_formats(void) { return supported_formats; }

static inline bool pointer_valid(const void *x, const char *name,
                                 const char *func) {
  if (!x) {
    blog(LOG_WARNING, "ols-scripting: [%s] %s is null", func, name);
    return false;
  }

  return true;
}

#define ptr_valid(x) pointer_valid(x, #x, __FUNCTION__)

ols_script_t *ols_script_create(const char *path, ols_data_t *settings) {
  ols_script_t *script = NULL;
  const char *ext;

  if (!scripting_loaded)
    return NULL;
  if (!ptr_valid(path))
    return NULL;

  ext = strrchr(path, '.');
  if (!ext)
    return NULL;

#if defined(LUAJIT_FOUND)
  if (strcmp(ext, ".lua") == 0) {
    script = ols_lua_script_create(path, settings);
  } else
#endif
#if defined(Python_FOUND)
      if (strcmp(ext, ".py") == 0) {
    script = ols_python_script_create(path, settings);
  } else
#endif
  {
    blog(LOG_WARNING, "Unsupported/unknown script type: %s", path);
  }

  return script;
}

void ols_scripting_prase(ols_script_t *script, const char *data, int len) {

  if (OLS_SCRIPT_LANG_LUA == script->type) {
    ols_lua_parse_data(script,data,len);
  }
}

const char *ols_script_get_description(const ols_script_t *script) {
  return ptr_valid(script) ? script->desc.array : NULL;
}

const char *ols_script_get_path(const ols_script_t *script) {
  const char *path = ptr_valid(script) ? script->path.array : "";
  return path ? path : "";
}

const char *ols_script_get_file(const ols_script_t *script) {
  const char *file = ptr_valid(script) ? script->file.array : "";
  return file ? file : "";
}

enum ols_script_lang ols_script_get_lang(const ols_script_t *script) {
  return ptr_valid(script) ? script->type : OLS_SCRIPT_LANG_UNKNOWN;
}

static void clear_queue_signal(void *p_event) {
  os_event_t *event = p_event;
  os_event_signal(event);
}

static void clear_call_queue(void) {
  os_event_t *event;
  if (os_event_init(&event, OS_EVENT_TYPE_AUTO) != 0)
    return;

  defer_call_post(clear_queue_signal, event);

  os_event_wait(event);
  os_event_destroy(event);
}

bool ols_script_reload(ols_script_t *script) {
  if (!scripting_loaded)
    return false;
  if (!ptr_valid(script))
    return false;

#if defined(LUAJIT_FOUND)
  if (script->type == OLS_SCRIPT_LANG_LUA) {
    ols_lua_script_unload(script);
    clear_call_queue();
    ols_lua_script_load(script);
    goto out;
  }
#endif
#if defined(Python_FOUND)
  if (script->type == OLS_SCRIPT_LANG_PYTHON) {
    ols_python_script_unload(script);
    clear_call_queue();
    ols_python_script_load(script);
    goto out;
  }
#endif

out:
  return script->loaded;
}

bool ols_script_loaded(const ols_script_t *script) {
  return ptr_valid(script) ? script->loaded : false;
}

void ols_script_destroy(ols_script_t *script) {
  if (!script)
    return;

#if defined(LUAJIT_FOUND)
  if (script->type == OLS_SCRIPT_LANG_LUA) {
    ols_lua_script_unload(script);
    ols_lua_script_destroy(script);
    return;
  }
#endif
#if defined(Python_FOUND)
  if (script->type == OLS_SCRIPT_LANG_PYTHON) {
    ols_python_script_unload(script);
    ols_python_script_destroy(script);
    return;
  }
#endif
}

#if !defined(Python_FOUND)
bool ols_scripting_load_python(const char *python_path) {
  UNUSED_PARAMETER(python_path);
  return false;
}

bool ols_scripting_python_loaded(void) { return false; }

bool ols_scripting_python_runtime_linked(void) { return (bool)true; }

void ols_scripting_python_version(char *version, size_t version_length) {
  UNUSED_PARAMETER(version_length);
  version[0] = 0;
}
#endif
