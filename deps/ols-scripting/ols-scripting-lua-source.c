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
#include "cstrcache.h"

#include <ols-module.h>

/* ========================================================================= */

static inline const char *get_table_string_(lua_State *script, int idx,
					    const char *name, const char *func)
{
	const char *str = "";

	lua_pushstring(script, name);
	lua_gettable(script, idx - 1);
	if (!lua_isstring(script, -1))
		warn("%s: no item '%s' of type %s", func, name, "string");
	else
		str = cstrcache_get(lua_tostring(script, -1));
	lua_pop(script, 1);

	return str;
}

static inline int get_table_int_(lua_State *script, int idx, const char *name,
				 const char *func)
{
	int val = 0;

	lua_pushstring(script, name);
	lua_gettable(script, idx - 1);
	val = (int)lua_tointeger(script, -1);
	lua_pop(script, 1);

	UNUSED_PARAMETER(func);

	return val;
}

static inline void get_callback_from_table_(lua_State *script, int idx,
					    const char *name, int *p_reg_idx,
					    const char *func)
{
	*p_reg_idx = LUA_REFNIL;

	lua_pushstring(script, name);
	lua_gettable(script, idx - 1);
	if (!lua_isfunction(script, -1)) {
		if (!lua_isnil(script, -1)) {
			warn("%s: item '%s' is not a function", func, name);
		}
		lua_pop(script, 1);
	} else {
		*p_reg_idx = luaL_ref(script, LUA_REGISTRYINDEX);
	}
}

#define get_table_string(script, idx, name) \
	get_table_string_(script, idx, name, __FUNCTION__)
#define get_table_int(script, idx, name) \
	get_table_int_(script, idx, name, __FUNCTION__)
#define get_callback_from_table(script, idx, name, p_reg_idx) \
	get_callback_from_table_(script, idx, name, p_reg_idx, __FUNCTION__)




/* ========================================================================= */

struct ols_lua_data;

struct ols_lua_source {
	struct ols_lua_script *data;

	lua_State *script;

	int func_create;
	int func_destroy;
	int func_get_properties;
	int func_update;


	pthread_mutex_t definition_mutex;
	struct ols_lua_data *first_source;

	struct ols_lua_source *next;
	struct ols_lua_source **p_prev_next;
};

extern pthread_mutex_t lua_source_def_mutex;
struct ols_lua_source *first_source_def = NULL;

struct ols_lua_data {
	ols_source_t *source;
	struct ols_lua_source *ls;
	int lua_data_ref;

	struct ols_lua_data *next;
	struct ols_lua_data **p_prev_next;
};

#define call_func(name, args, rets)                                \
	call_func_(ls->script, ls->func_##name, args, rets, #name, \
		   ls->display_name)
#define have_func(name) (ls->func_##name != LUA_REFNIL)
#define ls_push_data() \
	lua_rawgeti(ls->script, LUA_REGISTRYINDEX, ld->lua_data_ref)
#define ls_pop(count) lua_pop(ls->script, count)
#define lock_script()                                              \
	struct ols_lua_script *__data = ls->data;                  \
	struct ols_lua_script *__prev_script = current_lua_script; \
	current_lua_script = __data;                               \
	pthread_mutex_lock(&__data->mutex);
#define unlock_script()                       \
	pthread_mutex_unlock(&__data->mutex); \
	current_lua_script = __prev_script;

static const char *ols_lua_source_get_name(void *type_data)
{
	struct ols_lua_source *ls = type_data;
	return ls->display_name;
}




static void ols_lua_source_destroy(void *data)
{
	struct ols_lua_data *ld = data;
	struct ols_lua_source *ls = ld->ls;
	struct ols_lua_data *next;

	pthread_mutex_lock(&ls->definition_mutex);
	if (!ls->script)
		goto fail;
	if (!have_func(destroy))
		goto fail;

	lock_script();
	call_destroy(ld);
	unlock_script();

fail:
	next = ld->next;
	*ld->p_prev_next = next;
	if (next)
		next->p_prev_next = ld->p_prev_next;

	bfree(data);
	pthread_mutex_unlock(&ls->definition_mutex);
}


