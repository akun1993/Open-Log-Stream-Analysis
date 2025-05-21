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

#pragma once

/* ---------------------------- */

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4189)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
/* ---------------------------- */

#include <util/base.h>
#include <util/bmem.h>
#include <util/threading.h>

#include "ols-scripting-callback.h"
#include "ols-scripting-internal.h"

#define do_log(level, format, ...) blog(level, "[Lua] " format, ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

/* ------------------------------------------------------------ */

struct ols_lua_script;

extern THREAD_LOCAL struct ols_lua_script *current_lua_script;

/* ------------------------------------------------------------ */

struct ols_lua_script {
  ols_script_t base;

  struct dstr dir;
  struct dstr log_chunk;

  pthread_mutex_t mutex;
  lua_State *script;

  int update;
  int get_properties;
  int save;

  bool defined_sources;
};

static int is_ptr(lua_State *script, int idx) {
  return lua_isuserdata(script, idx) || lua_isnil(script, idx);
}

static int is_table(lua_State *script, int idx) {
  return lua_istable(script, idx);
}

static int is_function(lua_State *script, int idx) {
  return lua_isfunction(script, idx);
}

typedef int (*param_cb)(lua_State *script, int idx);

static inline bool verify_args1_(lua_State *script, param_cb param1_check,
                                 const char *func) {
  if (lua_gettop(script) != 1) {
    warn("Wrong number of parameters for %s", func);
    return false;
  }
  if (!param1_check(script, 1)) {
    warn("Wrong parameter type for parameter %d of %s", 1, func);
    return false;
  }

  return true;
}

#define verify_args1(script, param1_check)                                     \
  verify_args1_(script, param1_check, __FUNCTION__)

static inline bool call_func_(lua_State *script, int reg_idx, int args,
                              int rets, const char *func,
                              const char *display_name) {
  if (reg_idx == LUA_REFNIL)
    return false;

  struct ols_lua_script *data = current_lua_script;

  lua_rawgeti(script, LUA_REGISTRYINDEX, reg_idx);
  lua_insert(script, -1 - args);

  if (lua_pcall(script, args, rets, 0) != 0) {
    script_warn(&data->base, "Failed to call %s for %s: %s", func, display_name,
                lua_tostring(script, -1));
    lua_pop(script, 1);
    return false;
  }

  return true;
}
