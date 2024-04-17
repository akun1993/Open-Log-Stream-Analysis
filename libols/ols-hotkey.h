/******************************************************************************
    Copyright (C) 2014-2015 by Ruwen Hahn <palana@stunned.de>

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

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t ols_hotkey_id;
typedef size_t ols_hotkey_pair_id;

#ifndef SWIG
#define OLS_INVALID_HOTKEY_ID (~(ols_hotkey_id)0)
#define OLS_INVALID_HOTKEY_PAIR_ID (~(ols_hotkey_pair_id)0)
#else
const size_t OLS_INVALID_HOTKEY_ID = (size_t)-1;
const size_t OLS_INVALID_HOTKEY_PAIR_ID = (size_t)-1;
#endif

#define XINPUT_MOUSE_LEN 33

enum ols_key {
#define OLS_HOTKEY(x) x,
#include "ols-hotkeys.h"
#undef OLS_HOTKEY
	OLS_KEY_LAST_VALUE //not an actual key
};
typedef enum ols_key ols_key_t;

struct ols_key_combination {
	uint32_t modifiers;
	ols_key_t key;
};
typedef struct ols_key_combination ols_key_combination_t;

typedef struct ols_hotkey ols_hotkey_t;
typedef struct ols_hotkey_binding ols_hotkey_binding_t;

enum ols_hotkey_registerer_type {
	OLS_HOTKEY_REGISTERER_FRONTEND,
	OLS_HOTKEY_REGISTERER_SOURCE,
	OLS_HOTKEY_REGISTERER_OUTPUT,
	OLS_HOTKEY_REGISTERER_SERVICE,
};
typedef enum ols_hotkey_registerer_type ols_hotkey_registerer_t;

/* getter functions */

EXPORT ols_hotkey_id ols_hotkey_get_id(const ols_hotkey_t *key);
EXPORT const char *ols_hotkey_get_name(const ols_hotkey_t *key);
EXPORT const char *ols_hotkey_get_description(const ols_hotkey_t *key);
EXPORT ols_hotkey_registerer_t
ols_hotkey_get_registerer_type(const ols_hotkey_t *key);
EXPORT void *ols_hotkey_get_registerer(const ols_hotkey_t *key);
EXPORT ols_hotkey_id ols_hotkey_get_pair_partner_id(const ols_hotkey_t *key);

EXPORT ols_key_combination_t
ols_hotkey_binding_get_key_combination(ols_hotkey_binding_t *binding);
EXPORT ols_hotkey_id
ols_hotkey_binding_get_hotkey_id(ols_hotkey_binding_t *binding);
EXPORT ols_hotkey_t *
ols_hotkey_binding_get_hotkey(ols_hotkey_binding_t *binding);

/* setter functions */

EXPORT void ols_hotkey_set_name(ols_hotkey_id id, const char *name);
EXPORT void ols_hotkey_set_description(ols_hotkey_id id, const char *desc);
EXPORT void ols_hotkey_pair_set_names(ols_hotkey_pair_id id, const char *name0,
				      const char *name1);
EXPORT void ols_hotkey_pair_set_descriptions(ols_hotkey_pair_id id,
					     const char *desc0,
					     const char *desc1);

#ifndef SWIG
struct ols_hotkeys_translations {
	const char *insert;
	const char *del;
	const char *home;
	const char *end;
	const char *page_up;
	const char *page_down;
	const char *num_lock;
	const char *scroll_lock;
	const char *caps_lock;
	const char *backspace;
	const char *tab;
	const char *print;
	const char *pause;
	const char *left;
	const char *right;
	const char *up;
	const char *down;
	const char *shift;
	const char *alt;
	const char *control;
	const char *meta; /* windows/super key */
	const char *menu;
	const char *space;
	const char *numpad_num; /* For example, "Numpad %1" */
	const char *numpad_divide;
	const char *numpad_multiply;
	const char *numpad_minus;
	const char *numpad_plus;
	const char *numpad_decimal;
	const char *apple_keypad_num; /* For example, "%1 (Keypad)" */
	const char *apple_keypad_divide;
	const char *apple_keypad_multiply;
	const char *apple_keypad_minus;
	const char *apple_keypad_plus;
	const char *apple_keypad_decimal;
	const char *apple_keypad_equal;
	const char *mouse_num; /* For example, "Mouse %1" */
	const char *escape;
};

/* This function is an optional way to provide translations for specific keys
 * that may not have translations.  If the operating system can provide
 * translations for these keys, it will use the operating system's translation
 * over these translations.  If no translations are specified, it will use
 * the default English translations for that specific operating system. */
EXPORT void
ols_hotkeys_set_translations_s(struct ols_hotkeys_translations *translations,
			       size_t size);
#endif




/* registering hotkeys (giving hotkeys a name and a function) */

typedef void (*ols_hotkey_func)(void *data, ols_hotkey_id id,
				ols_hotkey_t *hotkey, bool pressed);

EXPORT ols_hotkey_id ols_hotkey_register_frontend(const char *name,
						  const char *description,
						  ols_hotkey_func func,
						  void *data);


EXPORT ols_hotkey_id ols_hotkey_register_output(ols_output_t *output,
						const char *name,
						const char *description,
						ols_hotkey_func func,
						void *data);



EXPORT ols_hotkey_id ols_hotkey_register_source(ols_source_t *source,
						const char *name,
						const char *description,
						ols_hotkey_func func,
						void *data);

typedef bool (*ols_hotkey_active_func)(void *data, ols_hotkey_pair_id id,
				       ols_hotkey_t *hotkey, bool pressed);

EXPORT ols_hotkey_pair_id ols_hotkey_pair_register_frontend(
	const char *name0, const char *description0, const char *name1,
	const char *description1, ols_hotkey_active_func func0,
	ols_hotkey_active_func func1, void *data0, void *data1);


EXPORT ols_hotkey_pair_id ols_hotkey_pair_register_output(
	ols_output_t *output, const char *name0, const char *description0,
	const char *name1, const char *description1,
	ols_hotkey_active_func func0, ols_hotkey_active_func func1, void *data0,
	void *data1);


EXPORT ols_hotkey_pair_id ols_hotkey_pair_register_source(
	ols_source_t *source, const char *name0, const char *description0,
	const char *name1, const char *description1,
	ols_hotkey_active_func func0, ols_hotkey_active_func func1, void *data0,
	void *data1);

EXPORT void ols_hotkey_unregister(ols_hotkey_id id);

EXPORT void ols_hotkey_pair_unregister(ols_hotkey_pair_id id);

/* loading hotkeys (associating a hotkey with a physical key and modifiers) */

EXPORT void ols_hotkey_load_bindings(ols_hotkey_id id,
				     ols_key_combination_t *combinations,
				     size_t num);

EXPORT void ols_hotkey_load(ols_hotkey_id id, ols_data_array_t *data);



EXPORT void ols_hotkeys_load_output(ols_output_t *output, ols_data_t *hotkeys);


EXPORT void ols_hotkeys_load_source(ols_source_t *source, ols_data_t *hotkeys);

EXPORT void ols_hotkey_pair_load(ols_hotkey_pair_id id, ols_data_array_t *data0,
				 ols_data_array_t *data1);

EXPORT ols_data_array_t *ols_hotkey_save(ols_hotkey_id id);

EXPORT void ols_hotkey_pair_save(ols_hotkey_pair_id id,
				 ols_data_array_t **p_data0,
				 ols_data_array_t **p_data1);


EXPORT ols_data_t *ols_hotkeys_save_output(ols_output_t *output);


EXPORT ols_data_t *ols_hotkeys_save_source(ols_source_t *source);

/* enumerating hotkeys */

typedef bool (*ols_hotkey_enum_func)(void *data, ols_hotkey_id id,
				     ols_hotkey_t *key);

EXPORT void ols_enum_hotkeys(ols_hotkey_enum_func func, void *data);

/* enumerating bindings */

typedef bool (*ols_hotkey_binding_enum_func)(void *data, size_t idx,
					     ols_hotkey_binding_t *binding);

EXPORT void ols_enum_hotkey_bindings(ols_hotkey_binding_enum_func func,
				     void *data);

/* hotkey event control */

EXPORT void ols_hotkey_inject_event(ols_key_combination_t hotkey, bool pressed);

EXPORT void ols_hotkey_enable_background_press(bool enable);

EXPORT void ols_hotkey_enable_strict_modifiers(bool enable);

/* hotkey callback routing (trigger callbacks through e.g. a UI thread) */

typedef void (*ols_hotkey_callback_router_func)(void *data, ols_hotkey_id id,
						bool pressed);

EXPORT void
ols_hotkey_set_callback_routing_func(ols_hotkey_callback_router_func func,
				     void *data);

EXPORT void ols_hotkey_trigger_routed_callback(ols_hotkey_id id, bool pressed);

/* hotkey callbacks won't be processed if callback rerouting is enabled and no
 * router func is set */
EXPORT void ols_hotkey_enable_callback_rerouting(bool enable);

/* misc */

typedef void (*ols_hotkey_atomic_update_func)(void *);
EXPORT void ols_hotkey_update_atomic(ols_hotkey_atomic_update_func func,
				     void *data);

struct dstr;
EXPORT void ols_key_to_str(ols_key_t key, struct dstr *str);
EXPORT void ols_key_combination_to_str(ols_key_combination_t key,
				       struct dstr *str);

EXPORT ols_key_t ols_key_from_virtual_key(int code);
EXPORT int ols_key_to_virtual_key(ols_key_t key);

EXPORT const char *ols_key_to_name(ols_key_t key);
EXPORT ols_key_t ols_key_from_name(const char *name);

static inline bool ols_key_combination_is_empty(ols_key_combination_t combo)
{
	return !combo.modifiers && combo.key == OLS_KEY_NONE;
}

#ifdef __cplusplus
}
#endif
