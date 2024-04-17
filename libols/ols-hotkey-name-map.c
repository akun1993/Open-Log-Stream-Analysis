/******************************************************************************
    Copyright (C) 2014 by Ruwen Hahn <palana@stunned.de>

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

#include <string.h>
#include <assert.h>

#include <util/bmem.h>
#include <util/c99defs.h>

#include "ols-internal.h"

struct ols_hotkey_name_map;

typedef struct ols_hotkey_name_map_item ols_hotkey_name_map_item_t;

struct ols_hotkey_name_map_item {
	char *key;
	int val;
	UT_hash_handle hh;
};

static void ols_hotkey_name_map_insert(ols_hotkey_name_map_item_t **hmap,
				       const char *key, int v)
{
	if (!hmap || !key)
		return;

	ols_hotkey_name_map_item_t *t;
	HASH_FIND_STR(*hmap, key, t);
	if (t)
		return;

	t = bzalloc(sizeof(ols_hotkey_name_map_item_t));
	t->key = bstrdup(key);
	t->val = v;

	HASH_ADD_STR(*hmap, key, t);
}

static bool ols_hotkey_name_map_lookup(ols_hotkey_name_map_item_t *hmap,
				       const char *key, int *v)
{
	if (!hmap || !key)
		return false;

	ols_hotkey_name_map_item_t *elem;

	HASH_FIND_STR(hmap, key, elem);

	if (elem) {
		*v = elem->val;
		return true;
	}

	return false;
}

static const char *ols_key_names[] = {
#define OLS_HOTKEY(x) #x,
#include "ols-hotkeys.h"
#undef OLS_HOTKEY
};

const char *ols_key_to_name(ols_key_t key)
{
	if (key >= OLS_KEY_LAST_VALUE) {
		blog(LOG_ERROR,
		     "ols-hotkey.c: queried unknown key "
		     "with code %d",
		     (int)key);
		return "";
	}

	return ols_key_names[key];
}

static ols_key_t ols_key_from_name_fallback(const char *name)
{
#define OLS_HOTKEY(x)              \
	if (strcmp(#x, name) == 0) \
		return x;
#include "ols-hotkeys.h"
#undef OLS_HOTKEY
	return OLS_KEY_NONE;
}

static void init_name_map(void)
{
#define OLS_HOTKEY(x) ols_hotkey_name_map_insert(&ols->hotkeys.name_map, #x, x);
#include "ols-hotkeys.h"
#undef OLS_HOTKEY
}

ols_key_t ols_key_from_name(const char *name)
{
	if (!ols)
		return ols_key_from_name_fallback(name);

	if (pthread_once(&ols->hotkeys.name_map_init_token, init_name_map))
		return ols_key_from_name_fallback(name);

	int v = 0;
	if (ols_hotkey_name_map_lookup(ols->hotkeys.name_map, name, &v))
		return v;

	return OLS_KEY_NONE;
}

void ols_hotkey_name_map_free(void)
{
	if (!ols || !ols->hotkeys.name_map)
		return;

	ols_hotkey_name_map_item_t *root = ols->hotkeys.name_map;
	ols_hotkey_name_map_item_t *n, *tmp;

	HASH_ITER (hh, root, n, tmp) {
		HASH_DEL(root, n);
		bfree(n->key);
		bfree(n);
	}
}
