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

#include "ols-scripting-lua.h"

#include <ols.h>
#include <util/base.h>
#include <util/dstr.h>
#include <util/platform.h>

/* ========================================================================= */

#if ARCH_BITS == 64
#define ARCH_DIR "64bit"
#else
#define ARCH_DIR "32bit"
#endif

#ifdef __APPLE__
#define SO_EXT "so"
#elif _WIN32
#include <windows.h>
#define SO_EXT "dll"
#else
#define SO_EXT "so"
#endif

static const char *startup_script_template =
    "\
for val in pairs(package.preload) do\n\
	package.preload[val] = nil\n\
end\n\
package.cpath = package.cpath .. \";\" .. \"%s/?." SO_EXT
    "\" .. \";\" .. \"%s\" .. \"/?." SO_EXT "\"\n\
require \"olslua\"\n";

static const char *get_script_path_func = "\
function script_path()\n\
	 return \"%s\"\n\
end\n\
package.path = package.path .. \";\" .. script_path() .. \"/?.lua\"\n";

static char *startup_script = NULL;

pthread_mutex_t lua_source_def_mutex;

#define call_func(name, args, rets)                                            \
  call_func_(script, cb->reg_idx, args, rets, #name, __FUNCTION__)

/* -------------------------------------------- */

THREAD_LOCAL struct lua_ols_callback *current_lua_cb = NULL;
THREAD_LOCAL struct ols_lua_script *current_lua_script = NULL;

/* -------------------------------------------- */

static int hook_print(lua_State *script) {
  struct ols_lua_script *data = current_lua_script;
  const char *msg = lua_tostring(script, 1);
  if (!msg)
    return 0;

  script_info(&data->base, "%s", msg);
  return 0;
}

static int hook_error(lua_State *script) {
  struct ols_lua_script *data = current_lua_script;
  const char *msg = lua_tostring(script, 1);
  if (!msg)
    return 0;

  script_error(&data->base, "%s", msg);
  return 0;
}

/* -------------------------------------------- */
static int lua_script_log(lua_State *script) {
  struct ols_lua_script *data = current_lua_script;
  int log_level = (int)lua_tointeger(script, 1);
  const char *msg = lua_tostring(script, 2);

  if (!msg)
    return 0;

  /* ------------------- */

  dstr_copy(&data->log_chunk, msg);

  const char *start = data->log_chunk.array;
  char *endl = strchr(start, '\n');

  while (endl) {
    *endl = 0;
    script_log(&data->base, log_level, "%s", start);
    *endl = '\n';

    start = endl + 1;
    endl = strchr(start, '\n');
  }

  if (start && *start)
    script_log(&data->base, log_level, "%s", start);
  dstr_resize(&data->log_chunk, 0);

  /* ------------------- */

  return 0;
}
/* -------------------------------------------- */

static void add_hook_functions(lua_State *script) {
#define add_func(name, func)                                                   \
  do {                                                                         \
    lua_pushstring(script, name);                                              \
    lua_pushcfunction(script, func);                                           \
    lua_rawset(script, -3);                                                    \
  } while (false)

  lua_getglobal(script, "_G");

  add_func("print", hook_print);
  add_func("error", hook_error);

  lua_pop(script, 1);

  /* ------------- */

  lua_getglobal(script, "olslua");

  add_func("script_log", lua_script_log);

  lua_pop(script, 1);
#undef add_func
}

/* -------------------------------------------- */

/* ========================================================================= */

static bool load_lua_script(struct ols_lua_script *data) {
  struct dstr str = {0};
  bool success = false;
  int ret;
  char *file_data;

  lua_State *script = luaL_newstate();
  if (!script) {
    script_warn(&data->base, "Failed to create new lua state");
    goto fail;
  }

  pthread_mutex_lock(&data->mutex);

  luaL_openlibs(script);
  luaopen_ffi(script);

  if (luaL_dostring(script, startup_script) != 0) {
    script_warn(&data->base, "Error executing startup script 1: %s",
                lua_tostring(script, -1));
    goto fail;
  }

  dstr_printf(&str, get_script_path_func, data->dir.array);
  ret = luaL_dostring(script, str.array);
  dstr_free(&str);

  if (ret != 0) {
    script_warn(&data->base, "Error executing startup script 2: %s",
                lua_tostring(script, -1));
    goto fail;
  }

  current_lua_script = data;

  add_hook_functions(script);
#ifdef ENABLE_UI
  // add_lua_frontend_funcs(script);
#endif

  file_data = os_quick_read_utf8_file(data->base.path.array);
  if (!file_data) {
    script_warn(&data->base, "Error opening file: %s",
                lua_tostring(script, -1));
    goto fail;
  }

  if (luaL_loadbuffer(script, file_data, strlen(file_data),
                      data->base.path.array) != 0) {
    script_warn(&data->base, "Error loading file: %s",
                lua_tostring(script, -1));
    bfree(file_data);
    goto fail;
  }
  bfree(file_data);

  if (lua_pcall(script, 0, LUA_MULTRET, 0) != 0) {
    script_warn(&data->base, "Error running file: %s",
                lua_tostring(script, -1));
    goto fail;
  }

  ret = lua_gettop(script);
  if (ret == 1 && lua_isboolean(script, -1)) {
    bool success = lua_toboolean(script, -1);

    if (!success) {
      goto fail;
    }
  }

  lua_getglobal(script, "script_properties");
  if (lua_isfunction(script, -1))
    data->get_properties = luaL_ref(script, LUA_REGISTRYINDEX);
  else
    data->get_properties = LUA_REFNIL;

  lua_getglobal(script, "script_update");
  if (lua_isfunction(script, -1))
    data->update = luaL_ref(script, LUA_REGISTRYINDEX);
  else
    data->update = LUA_REFNIL;

  lua_getglobal(script, "script_save");
  if (lua_isfunction(script, -1))
    data->save = luaL_ref(script, LUA_REGISTRYINDEX);
  else
    data->save = LUA_REFNIL;

  data->script = script;
  success = true;

fail:
  if (script) {
    lua_settop(script, 0);
    pthread_mutex_unlock(&data->mutex);
  }

  if (!success && script) {
    lua_close(script);
  }

  current_lua_script = NULL;
  return success;
}

bool ols_lua_script_load(ols_script_t *s) {
  struct ols_lua_script *data = (struct ols_lua_script *)s;
  if (!data->base.loaded) {
    data->base.loaded = load_lua_script(data);
    if (data->base.loaded) {
      blog(LOG_INFO, "[ols-scripting]: Loaded lua script: %s",
           data->base.file.array);
      // ols_lua_script_update(s, NULL);
    }
  }

  return data->base.loaded;
}

ols_script_t *ols_lua_script_create(const char *path, ols_data_t *settings) {
  struct ols_lua_script *data = bzalloc(sizeof(*data));

  data->base.type = OLS_SCRIPT_LANG_LUA;
  // data->tick = LUA_REFNIL;

  pthread_mutex_init_value(&data->mutex);

  if (pthread_mutex_init_recursive(&data->mutex) != 0) {
    bfree(data);
    return NULL;
  }

  dstr_copy(&data->base.path, path);

  char *slash = path && *path ? strrchr(path, '/') : NULL;
  if (slash) {
    slash++;
    dstr_copy(&data->base.file, slash);
    dstr_left(&data->dir, &data->base.path, slash - path);
  } else {
    dstr_copy(&data->base.file, path);
  }

  data->base.settings = ols_data_create();
  if (settings)
    ols_data_apply(data->base.settings, settings);

  ols_lua_script_load((ols_script_t *)data);
  return (ols_script_t *)data;
}

void ols_lua_script_unload(ols_script_t *s) {
  struct ols_lua_script *data = (struct ols_lua_script *)s;

  if (!s->loaded)
    return;

  lua_State *script = data->script;

  /* ---------------------------- */
  /* mark callbacks as removed    */

  pthread_mutex_lock(&data->mutex);

  /* XXX: scripts can potentially make callbacks when this happens, so
   * this probably still isn't ideal as we can't predict how the
   * processor or operating system is going to schedule things. a more
   * ideal method would be to reference count the script objects and
   * atomically share ownership with callbacks when they're called. */
  struct lua_ols_callback *cb = (struct lua_ols_callback *)data->first_callback;
  while (cb) {
    os_atomic_set_bool(&cb->base.removed, true);
    cb = (struct lua_ols_callback *)cb->base.next;
  }

  pthread_mutex_unlock(&data->mutex);

  /* ---------------------------- */
  /* call script_unload           */

  pthread_mutex_lock(&data->mutex);
  current_lua_script = data;

  lua_getglobal(script, "script_unload");
  lua_pcall(script, 0, 0, 0);

  current_lua_script = NULL;

  /* ---------------------------- */
  /* remove all callbacks         */

  cb = (struct lua_ols_callback *)data->first_callback;
  while (cb) {
    struct lua_ols_callback *next = (struct lua_ols_callback *)cb->base.next;
    remove_lua_ols_callback(cb);
    cb = next;
  }

  pthread_mutex_unlock(&data->mutex);

  /* ---------------------------- */
  /* close script                 */

  lua_close(script);
  s->loaded = false;

  blog(LOG_INFO, "[ols-scripting]: Unloaded lua script: %s",
       data->base.file.array);
}

void ols_lua_script_destroy(ols_script_t *s) {
  struct ols_lua_script *data = (struct ols_lua_script *)s;

  if (data) {
    pthread_mutex_destroy(&data->mutex);
    dstr_free(&data->base.path);
    dstr_free(&data->base.file);
    dstr_free(&data->base.desc);
    ols_data_release(data->base.settings);
    dstr_free(&data->log_chunk);
    dstr_free(&data->dir);
    bfree(data);
  }
}

/* -------------------------------------------- */

void ols_lua_load(void) {
  struct dstr tmp = {0};

  pthread_mutex_init(&lua_source_def_mutex, NULL);

  /* ---------------------------------------------- */
  /* Initialize Lua startup script                  */

#if _WIN32
#define PATH_MAX MAX_PATH
#endif

  char import_path[PATH_MAX];

#ifdef __APPLE__
  struct dstr bundle_path;

  dstr_init_move_array(&bundle_path, os_get_executable_path_ptr(""));
  dstr_cat(&bundle_path, "../PlugIns");
  char *absolute_plugin_path = os_get_abs_path_ptr(bundle_path.array);

  if (absolute_plugin_path != NULL) {
    strcpy(import_path, absolute_plugin_path);
    bfree(absolute_plugin_path);
  }
  dstr_free(&bundle_path);
#else
  strcpy(import_path, "./");
#endif
  dstr_printf(&tmp, startup_script_template, import_path, SCRIPT_DIR);
  startup_script = tmp.array;

  // ols_add_tick_callback(lua_tick, NULL);
}

void ols_lua_unload(void) {
  // ols_remove_tick_callback(lua_tick, NULL);

  bfree(startup_script);

  pthread_mutex_destroy(&lua_source_def_mutex);
}
